#!/usr/bin/env python
# -*- coding: utf-8 -*-

import subprocess
import time
import json

from tagconnector import TagConnector
from statistics import Statistics
from histogram import Histogram
from animator import Animator
from colormap import Colormap
from snapshot import Snapshot
from image import Image

class Cartavis:
    """
    A Python representation of the Carta application.

    Parameters
    ----------
    executable: string
        The full path of the Carta executable file.
    configFile: string
        The full path of the cartavis config.json file.
    port: integer
        The port which will be used to send commands to C++ and receive
        results.
    htmlFile: string
        The full path of the desktopIndex.html file.
    imageFile: string
        The full path of a compatible image file to load.
    """

    def __init__(self, executable, configFile, port, htmlFile, imageFile):
        args = [executable, "--scriptPort", 
                str(port), "--html", htmlFile, imageFile]
        print args
        self.popen = subprocess.Popen(args)
        print "Started process with pid=", self.popen.pid
        time.sleep(3)
        self.visible = False
        self.con = TagConnector(port)
        return

    def setGuiVisible(self,flag):
        """
        This command currently doesn't do anything useful.
        """
        self.visible = flag
        return

    def isGuiVisible(self):
        """
        This command currently doesn't do anything useful.
        """
        return self.visible

    def kill(self):
        """
        Closes the application.
        """
        self.popen.kill()

    def quit(self):
        """
        Closes the application.
        An alias for kill().
        """
        self.kill()

    def getColormaps(self):
        """
        Return a list of the names of colormaps available on the server.

        Returns
        -------
        list
            The names of colormaps available on the server.
        """
        commandStr = "getColormaps"
        colormapsList = self.con.cmdTagList(commandStr)
        return colormapsList

    def getImageViews(self):
        """
        Return a list of the image views defined by the layout.

        Returns
        -------
        list
            A list of Image objects.
        """
        commandStr = "getImageViews"
        imageViewsList = self.con.cmdTagList(commandStr)
        imageViews = []
        if (imageViewsList[0] != ""):
            for iv in imageViewsList:
                imageView = Image(iv, self.con)
                imageViews.append(imageView)
        return imageViews

    def getColormapViews(self):
        """
        Return a list of the colormap views defined by the layout.

        Returns
        -------
        list
            A list of Colormap objects.
        """
        commandStr = "getColormapViews"
        colormapViewsList = self.con.cmdTagList(commandStr)
        colormapViews = []
        if (colormapViewsList[0] != ""):
            for cmv in colormapViewsList:
                colormapView = Colormap(cmv, self.con)
                colormapViews.append(colormapView)
        return colormapViews

    def getAnimatorViews(self):
        """
        Return a list of the animator views defined by the layout.

        Returns
        -------
        list
            A list of Animator objects.
        """
        commandStr = "getAnimatorViews"
        animatorViewsList = self.con.cmdTagList(commandStr)
        animatorViews = []
        if (animatorViewsList[0] != ""):
            for av in animatorViewsList:
                animatorView = Animator(av, self.con)
                animatorViews.append(animatorView)
        return animatorViews

    def getHistogramViews(self):
        """
        Return a list of the histogram views defined by the layout.

        Returns
        -------
        list
            A list of Histogram objects.
        """
        commandStr = "getHistogramViews"
        histogramViewsList = self.con.cmdTagList(commandStr)
        histogramViews = []
        if (histogramViewsList[0] != ""):
            for hv in histogramViewsList:
                histogramView = Histogram(hv, self.con)
                histogramViews.append(histogramView)
        return histogramViews

    def getStatisticsViews(self):
        """
        Return a list of the statistics views defined by the layout.

        Returns
        -------
        list
            A list of Statistics objects.
        """
        commandStr = "getStatisticsViews"
        statisticsViewsList = self.con.cmdTagList(commandStr)
        statisticsViews = []
        if (statisticsViewsList[0] != ""):
            for sv in statisticsViewsList:
                statisticsView = Statistics(sv, self.con)
                statisticsViews.append(statisticsView)
        return statisticsViews

    def setAnalysisLayout(self):
        """
        Set the layout to a predefined analysis layout.

        Returns
        -------
        list
            An empty list, as the C++ methods that it calls have void
            return values.
        """
        commandStr = "setAnalysisLayout"
        result = self.con.cmdTagList(commandStr)
        return result

    def setImageLayout(self):
        """
        Set the layout to a predefined layout displaying a single image.

        Returns
        -------
        list
            An empty list, as the C++ methods that it calls have void
            return values.
        """
        commandStr = "setImageLayout"
        result = self.con.cmdTagList(commandStr)
        return result

    def setCustomLayout(self, rows, cols):
        """
        Set the number of rows and columns in the layout grid.

        Parameters
        ----------
        rows: integer
            The number of rows in the layout grid.
        cols: integer
            The number of columns in the layout grid.

        Returns
        -------
        list
            An error message if there was a problem setting the layout;
            empty otherwise.
        """
        result = self.con.cmdTagList("setCustomLayout", nrows=rows, ncols=cols)
        return result

    def setPlugins(self, pluginList):
        """
        Set plugins for each of the views in the layout.

        Parameters
        ----------
        pluginList: list
            A list of strings representing plugin names.
            Valid plugin names [NOTE: this list may not be complete]:
                CasaImageLoader
                Animator
                Statistics
                Colormap
                Histogram
                Hidden

        Returns
        -------
        list
            An error message if there was a problem setting the plugins;
            empty otherwise.
        """
        pluginString = ' '.join(pluginList)
        result = self.con.cmdTagList("setPlugins", plugins=pluginString)
        return result

    def addLink(self, source, dest):
        """
        Establish a link between a source and destination.
        The parameters are CartaView objects that are obtained by
        commands such as getImageViews() and getColormapViews(). For
        example, the following sequence of commands will obtain the
        image views and histogram views, then create a link between the
        first image view and the first histogram view:

            i = v.getImageViews()
            h = v.getHistogramViews()
            v.addLink(i[0], h[0])

        Parameters
        ----------
        source: CartaView object
            The source object for the link.
        dest: CartaView object
            The destination object for the link.

        Returns
        -------
        list
            An error message if there was a problem adding the link;
            empty otherwise.
        """
        result = self.con.cmdTagList("addLink", sourceView=source.getId(),
                                     destView=dest.getId())
        return result

    def removeLink(self, source, dest):
        """
        Remove a link from a source to a destination.
        The parameters are CartaView objects that are obtained by
        commands such as getImageViews() and getColormapViews(). For
        example, the following sequence of commands will obtain the
        image views and histogram views, then remove the link between
        the first image view and the first histogram view:

            i = v.getImageViews()
            h = v.getHistogramViews()
            v.removeLink(i[0], h[0])

        Parameters
        ----------
        source: CartaView object
            The source object for the link.
        dest: CartaView object
            The destination object for the link.

        Returns
        -------
        list
            An error message if there was a problem removing the link;
            empty otherwise.
        """
        result = self.con.cmdTagList("removeLink", sourceView=source.getId(),
                                     destView=dest.getId())
        return result

    def newSnapshot(self, sessionId, saveName, saveLayout, savePreferences,
                     saveData, description):
        """
        Create a new Snapshot object that can be saved.

        Parameters
        ---------
        sessionId: string
            An identifier for a user session.

        saveName: string
            The name of the snapshot.

        saveLayout: boolean
            True if the layout is to be saved; false otherwise.

        savePreferences: boolean
            True if the preferences are to be saved; false otherwise.

        saveData: boolean
            True if the data is to be saved; false otherwise.

        description: string
            Descriptive information about the snapshot.

        Returns
        -------
        Snapshot
            A new instance of the Snapshot class.
        """
        snapshot = Snapshot(sessionId=sessionId, snapshotType='', index=0,
                            saveName=saveName, layout=saveLayout,
                            preferences=savePreferences, data=saveData,
                            description=description, dateCreated='',
                            connection=self.con)
        return snapshot

    def getSnapshots(self, sessionId):
        """
        Get a list of Snapshot objects.

        Parameters
        ----------
        sessionId: string
            An identifier for a user session.

        Returns
        -------
        list
            A list of Snapshot objects.
        """
        snapshots = self.con.cmdTagList("getSnapshotObjects",
                                        sessionId=sessionId)
        snapshotObjects = []
        if (snapshots[0] != ""):
            for snapshot in snapshots:
                j = json.loads(snapshot)
                snapObj = Snapshot(sessionId, j['type'], j['index'],
                                   j['Snapshot'], j['layout'],
                                   j['preferences'], j['data'],
                                   j['description'], j['dateCreated'],
                                   self.con)
                snapshotObjects.append(snapObj)
        return snapshotObjects

    def fakeCommand(self, infile):
        """
        Purely for the purpose of testing what happens when an
        arbitrarily large command is sent.
        """
        f = open(infile, 'r')
        print "Start time: " + time.asctime()
        result = self.con.cmdTagList("fakeCommand", data=f.read())
        print "Finish time: " + time.asctime()
        return result

    def uc(self):
        """
        A command that is not implemented on the C++ side.
        For testing purposes only.
        """
        result = self.con.cmdTagList("unknownCommand")
        return result
