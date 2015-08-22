/**
 * Displays controls for customizing a particular contour set.
 */
/*global mImport */
/*******************************************************************************
 * @ignore( mImport)
 ******************************************************************************/

qx.Class.define("skel.widgets.Image.Contour.ContourSetPage", {
    extend : qx.ui.tabview.Page,

    /**
     * Constructor.
     */
    construct : function( ) {
        this.base(arguments, "Contour Set", "");
        this._init( );
    },
    
    events : {
        "contourSetChanged" : "qx.event.type.Data"
    },

    members : {
        
        /**
         * Initializes the UI.
         */
        _init : function( ) {
            this.setPadding( 0, 0, 0, 0 );
            this.setMargin( 1, 1, 1, 1 );
            this._setLayout(new qx.ui.layout.HBox(2));
            
            //Level list
            this.m_levelsList = new skel.widgets.Image.Contour.LevelTable();
            this.m_levelsList.addListener( "levelsChanged", function( evt ){
                this._levelsChanged( evt );
            }, this );
            this.m_levelsList.addListener( "levelSelected", function( evt ){
                this._levelSelected( evt );
            }, this );
            
            this.m_contourWidget = new skel.widgets.Image.Contour.ContourWidget();
            this.m_contourWidget.setContourSetName( this.getLabel() );
            
            this._add( this.m_levelsList );
            this._add( this.m_contourWidget );
        },
        
        /**
         * Fire a contour set changed event.
         * Include our id - the contour set index
         */
        _levelsChanged : function(  ){
            
            
            var contourData = {
                contourSetIndex : this.m_id
            };
            this.fireDateEvent( "contourSetChanged", contourData );
        },
        
        /**
         * Update the displayed contour information based upon the level that
         * was selected.
         */
        _levelSelected : function( evt ){
            var data = evt.getData();
            var selectIndex = -1;
            for ( var i = 0; i < this.m_contours.length; i++ ){
                if ( data.level == this.m_contours[i].level ){
                    selectIndex = i;
                    break;
                }
            }
            if ( selectIndex >= 0 ){
                this.m_contourWidget.setContour( this.m_contours[selectIndex] );
            }
        },
        
        /**
         * Update from the server when the grid controls have changed.
         * @param controls {Object} - information about the grid controls from the server.
         */
        setControls : function( controls ){
            this.m_contours = controls.contours;
            this.setLabel( controls.name );
            this._updateLevels();
            if ( this.m_contours.length > 0 ){
                this.m_contourWidget.setContour( this.m_contours[0], 0);
            }
        },
        
        /**
         * Set the server side id of this control UI.
         * @param contourId {String} the server side id of the object that contains 
         *      data for this control UI.
         */
        setId : function( contourId ){
            this.m_id = contourId;
            this.m_contourWidget.setId( contourId );
        },
        
        /**
         * Update the list of contour levels based on existing contour sets.
         */
        _updateLevels : function(){
            var levels = [];
            for ( var i = 0; i < this.m_contours.length; i++ ){
                levels[i] = this.m_contours[i].level;
            }
            this.m_levelsList.setLevels( levels );
        },
        
        m_id : null,
        m_connector : null,

        m_levelsList : null,
        m_contourWidget : null,
        
        m_contours : null
    }
});