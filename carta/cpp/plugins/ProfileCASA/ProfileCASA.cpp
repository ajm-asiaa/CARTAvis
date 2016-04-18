#include "ProfileCASA.h"
#include "plugins/CasaImageLoader/CCImage.h"
#include "plugins/CasaImageLoader/CCMetaDataInterface.h"
#include "CartaLib/Hooks/Initialize.h"
#include "CartaLib/Hooks/ProfileHook.h"
#include "CartaLib/RegionInfo.h"
#include "CartaLib/ProfileInfo.h"
#include "CartaLib/IImage.h"
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <images/Regions/WCEllipsoid.h>
#include <images/Regions/RegionManager.h>

#include <iterator>
using namespace std;
#include <imageanalysis/ImageAnalysis/PixelValueManipulator.h>
#include <imageanalysis/ImageAnalysis/PixelValueManipulatorData.h>
#include <imageanalysis/ImageAnalysis/ImageCollapserData.h>


#include <QDebug>


ProfileCASA::ProfileCASA(QObject *parent) :
    QObject(parent){
}


casa::MFrequency::Types ProfileCASA::_determineRefFrame(
        std::shared_ptr<casa::ImageInterface<casa::Float> > img ) const {
    casa::MFrequency::Types freqtype = casa::MFrequency::DEFAULT;
    if ( img ){
        casa::CoordinateSystem cSys=img->coordinates();
        casa::Int specAx=cSys.findCoordinate(casa::Coordinate::SPECTRAL);
        if ( specAx >= 0 ) {

            casa::SpectralCoordinate specCoor=cSys.spectralCoordinate(specAx);
            casa::MFrequency::Types tfreqtype;
            casa::MEpoch tepoch;
            casa::MPosition tposition;
            casa::MDirection tdirection;
            specCoor.getReferenceConversion(tfreqtype, tepoch, tposition, tdirection);
            freqtype = specCoor.frequencySystem(casa::False); // false means: get the native type
        }
    }
    return freqtype;
}

std::vector<double> ProfileCASA::_generateProfile( casa::ImageInterface < casa::Float > * imagePtr,
        Carta::Lib::RegionInfo regionInfo, Carta::Lib::ProfileInfo profileInfo ) const {
    std::vector<double> profileData;
    casa::CoordinateSystem cSys = imagePtr->coordinates();
    casa::uInt spectralAxis = 0;
    if ( cSys.hasSpectralAxis()){
        spectralAxis = cSys.spectralAxisNumber();
    }
    else {
        int tabCoord = cSys.findCoordinate( casa::Coordinate::TABULAR );
        if ( tabCoord >= 0 ){
            spectralAxis = tabCoord;
        }

    }
    Carta::Lib::RegionInfo::RegionType shape = regionInfo.getRegionType();
    std::vector<std::pair<double,double> > regionCorners = regionInfo.getCorners();
    int cornerCount = regionCorners.size();
    casa::Vector<casa::Double> x(cornerCount);
    casa::Vector<casa::Double> y(cornerCount);
    for ( int i = 0; i < cornerCount; i++ ){
        x[i] = regionCorners[i].first;
        y[i] = regionCorners[i].second;
    }
    casa::Record regionRecord = _getRegionRecord( shape, cSys, x, y);

    casa::String pixelSpectralType( "default" );
    casa::String unit( "pixel" );

    casa::PixelValueManipulatorData::SpectralType specType
        = casa::PixelValueManipulatorData::spectralType( pixelSpectralType );
    casa::Vector<casa::Float> jyValues;
    try {
        std::shared_ptr<casa::ImageInterface<casa::Float> >image ( imagePtr->cloneII() );
        casa::PixelValueManipulator<casa::Float> pvm(image, &regionRecord, "");
        casa::ImageCollapserData::AggregateType funct = _getCombineMethod( profileInfo );
        casa::MFrequency::Types freqType = _determineRefFrame( image );
        casa::String frame = casa::String( casa::MFrequency::showType( freqType));
        double restFrequency = profileInfo.getRestFrequency();
        QString restUnit = profileInfo.getRestUnit();
        casa::Quantity restFreq( restFrequency, casa::Unit( restUnit.toStdString().c_str()));
        casa::Record result = pvm.getProfile( spectralAxis, funct, unit, specType,
                &restFreq, frame );
        const casa::String VALUE_KEY( "values");
        if ( result.isDefined( VALUE_KEY )){
            result.get( VALUE_KEY, jyValues );
        }

        int dataCount = jyValues.size();
        for ( int i = 0; i < dataCount; i++ ){
            profileData.push_back( jyValues[i] );
        }

    }
    catch( casa::AipsError& error ){
        qDebug() << "Could not generate profile: "<<error.getMesg().c_str();
    }
    return profileData;
}


casa::ImageCollapserData::AggregateType ProfileCASA::_getCombineMethod( Carta::Lib::ProfileInfo profileInfo ) const {
    Carta::Lib::ProfileInfo::AggregateType combineType = profileInfo.getAggregateType();
    casa::ImageCollapserData::AggregateType collapseType = casa::ImageCollapserData::AggregateType::MEAN;
    if ( combineType == Carta::Lib::ProfileInfo::AggregateType::MEDIAN ){
        collapseType = casa::ImageCollapserData::AggregateType::MEDIAN;
    }
    else if ( combineType == Carta::Lib::ProfileInfo::AggregateType::SUM ){
        collapseType = casa::ImageCollapserData::AggregateType::SUM;
    }
    else if ( combineType == Carta::Lib::ProfileInfo::AggregateType::FLUX_DENSITY ){
        collapseType = casa::ImageCollapserData::AggregateType::FLUX;
    }
    return collapseType;
}


casa::ImageRegion* ProfileCASA::_getEllipsoid(const casa::CoordinateSystem& cSys,
        const casa::Vector<casa::Double>& x, const casa::Vector<casa::Double>& y) const {
    casa::Vector<casa::Quantity> center(2);
    casa::Vector<casa::Quantity> radius(2);
    casa::ImageRegion* imageRegion = NULL;
    if ( x.size() == 2 && y.size() == 2 ){
        const casa::String radUnits( "rad");
        center[0] = casa::Quantity( (x[0]+x[1])/2, radUnits );
        center[1] = casa::Quantity( (y[0]+y[1])/2, radUnits );

        casa::MDirection::Types type = casa::MDirection::N_Types;
        int directionIndex = cSys.findCoordinate( casa::Coordinate::DIRECTION );
        if ( directionIndex >= 0 ){
            casa::uInt dirIndex = static_cast<casa::uInt>(directionIndex);
            type = cSys.directionCoordinate(dirIndex).directionType(true);

            casa::Vector<casa::Double> qCenter(2);
            qCenter[0] = center[0].getValue();
            qCenter[1] = center[1].getValue();
            casa::MDirection mdcenter( casa::Quantum<casa::Vector<casa::Double> >(qCenter,radUnits), type );

            casa::Vector<casa::Double> blc_rad_x(2);
            blc_rad_x[0] = x[0];
            blc_rad_x[1] = center[1].getValue();
            casa::MDirection mdblc_x( casa::Quantum<casa::Vector<casa::Double> >(blc_rad_x,radUnits),type );

            casa::Vector<casa::Double> blc_rad_y(2);
            blc_rad_y[0] = center[0].getValue();
            blc_rad_y[1] = y[0];
            casa::MDirection mdblc_y( casa::Quantum<casa::Vector<casa::Double> >(blc_rad_y,radUnits),type );

            double xdistance = mdcenter.getValue( ).separation(mdblc_x.getValue( ));
            double ydistance = mdcenter.getValue( ).separation(mdblc_y.getValue( ));
            const float ERR = 0;
            if ( xdistance > ERR && ydistance > ERR ){
                radius[0] = casa::Quantity(xdistance, radUnits );
                radius[1] = casa::Quantity(ydistance, radUnits );

                casa::Vector<casa::Int> pixax(2);
                casa::Vector<casa::Int> dirPixelAxis = cSys.pixelAxes(directionIndex);
                pixax(0) = dirPixelAxis[0];
                pixax(1) = dirPixelAxis[1];
                casa::WCEllipsoid ellipsoid( center, radius, casa::IPosition(dirPixelAxis), cSys);
                imageRegion = new casa::ImageRegion( ellipsoid );
            }
        }
    }
    else {
        qDebug() << "Invalid size (2) for an ellipse: "<<x.size()<<" and "<<y.size();
    }
    return imageRegion;
}


std::vector<HookId> ProfileCASA::getInitialHookList(){
    return {
        Carta::Lib::Hooks::Initialize::staticId,
        Carta::Lib::Hooks::ProfileHook::staticId
    };
}


casa::ImageRegion* ProfileCASA::_getPolygon(const casa::CoordinateSystem& cSys,
        const casa::Vector<casa::Double>& x, const casa::Vector<casa::Double>& y) const {
    casa::ImageRegion* polygon = NULL;
    const casa::String radUnits( "rad");
    casa::RegionManager regMan;
    int n = x.size();
    casa::Vector<casa::Quantity> xvertex(n);
    casa::Vector<casa::Quantity> yvertex(n);
    for (casa::Int k = 0; k < n; ++k) {
        xvertex[k] = casa::Quantity(x[k], radUnits);
        yvertex[k] = casa::Quantity(y[k], radUnits);
    }
    int directionIndex = cSys.findCoordinate( casa::Coordinate::DIRECTION );
    if ( directionIndex >= 0 ){
        casa::Vector<casa::Int> dirPixelAxis = cSys.pixelAxes(directionIndex);
        casa::Vector<casa::Int> pixax(2);
        pixax(0) = dirPixelAxis[0];
        pixax(1) = dirPixelAxis[1];
        polygon = regMan.wpolygon(xvertex, yvertex, pixax, cSys, "abs");
    }
    return polygon;
}


casa::Record ProfileCASA::_getRegionRecord( Carta::Lib::RegionInfo::RegionType shape, const casa::CoordinateSystem& cSys,
        const casa::Vector<casa::Double>& x, const casa::Vector<casa::Double>& y) const {
    const casa::String radUnits( "rad");
    const casa::String absStr( "abs");
    casa::Record regionRecord;
    casa::Int directionIndex = cSys.findCoordinate(casa::Coordinate::DIRECTION);
    if ( directionIndex >= 0 ){
        casa::Vector<casa::Int> dirPixelAxis = cSys.pixelAxes(directionIndex);
        casa::RegionManager regMan;
        if ( shape == Carta::Lib::RegionInfo::RegionType::Polygon ){
            int ptCount = x.size();
            if ( ptCount == 2 ){
                casa::Vector<casa::Quantity> blc(2);
                casa::Vector<casa::Quantity> trc(2);
                blc(0) = casa::Quantity(x[0], radUnits);
                blc(1) = casa::Quantity(y[0], radUnits);
                trc(0) = casa::Quantity(x[1], radUnits);
                trc(1) = casa::Quantity(y[1], radUnits);
                casa::Vector<casa::Int> pixax(2);
                pixax(0) = dirPixelAxis[0];
                pixax(1) = dirPixelAxis[1];

                casa::Record* imagregRecord = regMan.wbox(blc, trc, pixax, cSys, absStr, radUnits);
                regionRecord = *imagregRecord;
                delete imagregRecord;
            }
            else if ( ptCount == 1 ){
                //Try a rectangle with blc=trc;
                casa::Vector<casa::Quantity> blc(2);
                casa::Vector<casa::Quantity> trc(2);
                blc(0) = casa::Quantity(x[0], radUnits);
                blc(1) = casa::Quantity(y[0], radUnits);
                trc(0) = casa::Quantity(x[0], radUnits);
                trc(1) = casa::Quantity(y[0], radUnits);
                casa::Vector<casa::Int> pixax(2);
                pixax(0) = dirPixelAxis[0];
                pixax(1) = dirPixelAxis[1];

                casa::Record* imagregRecord = regMan.wbox(blc, trc, pixax, cSys, absStr, radUnits);
                regionRecord=*imagregRecord;
                delete imagregRecord;
            }
            else if ( ptCount > 0 ){
                casa::ImageRegion* polygon = _getPolygon( cSys, x, y );
                if ( polygon != NULL ){
                    regionRecord = polygon->toRecord(casa::String(""));
                    delete polygon;
                }
            }
            else {
                qDebug() << "Profile Error: A region must have at least one point.";
            }
        }
        else if ( shape == Carta::Lib::RegionInfo::RegionType::Ellipse ){
            casa::ImageRegion* ellipsoid = _getEllipsoid( cSys, x, y );
            if ( ellipsoid != NULL ){
                regionRecord = ellipsoid->toRecord("");
                delete ellipsoid;
            }
        }
    }
    return regionRecord;
}


bool ProfileCASA::handleHook(BaseHook & hookData){
    qDebug() << "ProfileCASA plugin is handling hook #" << hookData.hookId();
    if( hookData.is<Carta::Lib::Hooks::Initialize>()) {
        return true;
    }
    else if ( hookData.is<Carta::Lib::Hooks::ProfileHook>()){
        Carta::Lib::Hooks::ProfileHook & hook
            = static_cast<Carta::Lib::Hooks::ProfileHook &>( hookData);

        std::shared_ptr<Carta::Lib::Image::ImageInterface> imagePtr = hook.paramsPtr->m_dataSource;

        if ( !imagePtr ) {
            return false;
        }

        casa::ImageInterface < casa::Float > * casaImage = cartaII2casaII_float( imagePtr );
        if( ! casaImage) {
            qWarning() << "Profile plugin: not an image created by casaimageloader...";
            return false;
        }

        Carta::Lib::RegionInfo regionInfo = hook.paramsPtr->m_regionInfo;
        Carta::Lib::ProfileInfo profileInfo = hook.paramsPtr->m_profileInfo;
        hook.result = _generateProfile( casaImage, regionInfo, profileInfo );
        return true;
    }
    qWarning() << "Sorry, ProfileCASA doesn't know how to handle this hook";
    return false;
}


ProfileCASA::~ProfileCASA(){

}