/***********************************************************************
KinectProjector - KinectProjector takes care of the spatial conversion
between the various coordinate systems, control the Kinectgrabber and
perform the calibration of the kinect and projector.
Copyright (c) 2016-2017 Thomas Wolf and Rasmus R. Paulsen (people.compute.dtu.dk/rapa)

This file is part of the Magic Sand.

The Magic Sand is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Magic Sand is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Magic Sand; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef __GreatSand__rs2Projector__
#define __GreatSand__rs2Projector__

#include <iostream>
#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "Rs2Grabber.h"
#include "ofxModal.h"

#include "Rs2ProjectorCalibration.h"
#include "Utils.h"
#include "TemporalFrameFilter.h"

class ofxModalThemeProjRs2 : public ofxModalTheme {
public:
    ofxModalThemeProjRs2()
    {
        animation.speed = 0.1f;
        fonts.title = ofxSmartFont::add("ofxbraitsch/fonts/HelveticaNeueLTStd-Md.otf", 20, "modal-title");
        fonts.message = ofxSmartFont::add("ofxbraitsch/fonts/Roboto-Regular.ttf", 16, "modal-message");
    }
};

class Rs2Projector {
public:
    Rs2Projector(std::shared_ptr<ofAppBaseWindow> const& p);
    
    // Running loop functions
    void setup(bool sdisplayGui);
    void update();
    void updateNativeScale(float scaleMin, float scaleMax);
    void drawProjectorWindow();
    void drawMainWindow(float x, float y, float width, float height);
    void drawGradField();

    // Coordinate conversion functions
    ofVec2f worldCoordToProjCoord(ofVec3f vin);
	ofVec3f projCoordAndWorldZToWorldCoord(float projX, float projY, float worldZ);
	ofVec2f rs2CoordToProjCoord(float x, float y);
	ofVec2f rs2CoordToProjCoord(float x, float y, float z);
	
	ofVec3f rs2CoordToWorldCoord(float x, float y);
	ofVec2f worldCoordTors2Coord(ofVec3f wc);
	ofVec3f Rawrs2CoordToWorldCoord(float x, float y);
    float elevationAtrs2Coord(float x, float y);
    float elevationTors2Depth(float elevation, float x, float y);
    ofVec2f gradientAtrs2Coord(float x, float y);

	// Try to start the application - assumes calibration has been done before
	void startApplication();

	// Setup & calibration functions
    void startFullCalibration();
    void startAutomaticROIDetection();
    void startAutomaticrs2ProjectorCalibration();
    void setGradFieldResolution(int gradFieldResolution);
	void updateStatusGUI();
	void setSpatialFiltering(bool sspatialFiltering);
	void setInPainting(bool inp);
	void setFullFrameFiltering(bool ff);	
	
	void setFollowBigChanges(bool sfollowBigChanges);
	void StartManualROIDefinition();
	void ResetSeaLevel();
	void showROIonProjector(bool show);

    // Gui and event functions
    void setupGui();
    void onButtonEvent(ofxDatGuiButtonEvent e);

	void onToggleEvent(ofxDatGuiToggleEvent e);
    void onSliderEvent(ofxDatGuiSliderEvent e);
    void onConfirmModalEvent(ofxModalEvent e);
    void onCalibModalEvent(ofxModalEvent e);

	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseDragged(int x, int y, int button);

    // Functions for shaders
    void bind(){
        FilteredDepthImage.getTexture().bind();
    }
    void unbind(){
        FilteredDepthImage.getTexture().unbind();
    }
    ofMatrix4x4 getTransposedRs2WorldMatrix(){
        return rs2WorldMatrix.getTransposedOf(rs2WorldMatrix);
    } // For shaders: OpenGL is row-major order and OF is column-major order
    ofMatrix4x4 getTransposedRs2ProjMatrix(){
        return rs2ProjMatrix.getTransposedOf(rs2ProjMatrix);
    }
	// Depending on the mount direction of the RealSense2, projections can be flipped.
	bool getProjectionFlipped();


    // Getter and setter
    ofTexture & getTexture(){
        return FilteredDepthImage.getTexture();
    }
    ofRectangle getRs2ROI(){
        return rs2ROI;
    }
    ofVec2f getRs2Res(){
        return rs2Res;
    }
    ofVec4f getBasePlaneEq(){
        return basePlaneEq;
    }
    ofVec3f getBasePlaneNormal(){
        return basePlaneNormal;
    }
    ofVec3f getBasePlaneOffset(){
        return basePlaneOffset;
    }

	// Get the ROI of the projector window that should match the RealSense2 ROI
	ofRectangle getProjectorActiveROI();

	// Map Game interface
	bool getBinaryLandImage(ofxCvGrayscaleImage& BinImg);

	bool isCalibrated(){
        return projRs2Calibrated;
    }
    bool isImageStabilized(){
        return imageStabilized;
    }
    bool isBasePlaneUpdated(){ // To be called after update()
        return basePlaneUpdated;
    }
    bool isCalibrationUpdated(){ // To be called after update()
        return projRs2CalibrationUpdated;
    }

	// The overall application stat
	enum Application_state
	{
		APPLICATION_STATE_IDLE,
		APPLICATION_STATE_SETUP,  // Shows depth image and display white rectangle on projector
		APPLICATION_STATE_CALIBRATING,
		APPLICATION_STATE_RUNNING
	};

	Application_state GetApplicationState()
	{
		return applicationState;
	}

	bool getDumpDebugFiles();

	// Debug functions
	void SaveFilteredDepthImage();
	void SaveRs2ColorImage();

private:

    enum Calibration_state
    {
        CALIBRATION_STATE_FULL_AUTO_CALIBRATION,
        CALIBRATION_STATE_ROI_AUTO_DETERMINATION,
        CALIBRATION_STATE_ROI_MANUAL_DETERMINATION,
		CALIBRATION_STATE_ROI_FROM_FILE,
		CALIBRATION_STATE_PROJ_RS2_AUTO_CALIBRATION,
        CALIBRATION_STATE_PROJ_RS2_MANUAL_CALIBRATION
    };
    enum Full_Calibration_state
    {
        FULL_CALIBRATION_STATE_ROI_DETERMINATION,
        FULL_CALIBRATION_STATE_AUTOCALIB,
        FULL_CALIBRATION_STATE_DONE
    };
    enum ROI_calibration_state
    {
        ROI_CALIBRATION_STATE_INIT,
        ROI_CALIBRATION_STATE_READY_TO_MOVE_UP,
        ROI_CALIBRATION_STATE_MOVE_UP,
        ROI_CALIBRATION_STATE_DONE
    };
    enum Auto_calibration_state
    {
        AUTOCALIB_STATE_INIT_FIRST_PLANE,
        AUTOCALIB_STATE_INIT_POINT,
        AUTOCALIB_STATE_NEXT_POINT,
        AUTOCALIB_STATE_COMPUTE,
        AUTOCALIB_STATE_DONE
    };

   
    void exit(ofEventArgs& e);
    void setupGradientField();
    
    
    void init_FBOprojector();
    void updateCalibration();
    void updateFullAutoCalibration();
    void updateROIAutoCalibration();
    void updateROIFromColorImage();
    void updateROIFromDepthImage();
	void updateROIFromFile();
	
//	void updateROIManualCalibration();
    void updateROIFromCalibration();
    void setMaxRs2GrabberROI();
    void setNewRs2ROI();
    void updateRs2GrabberROI(ofRectangle ROI);

	void updateProjRs2AutoCalibration();

	double ComputeReprojectionError(bool WriteFile);
	void CalibrateNextPoint();

	void updateProjRs2ManualCalibration();
    bool addPointPair();
    void updateMaxOffset();
    void updateBasePlane();
    void askToFlattenSand();

    void drawChessboard(int x, int y, int chessboardSize);
    void drawArrow(ofVec2f projectedPoint, ofVec2f v1);

    void saveCalibrationAndSettings();
    bool loadSettings();
    bool saveSettings();
    
	void ProcessChessBoardInput(ofxCvGrayscaleImage& image);
	void CheckAndNormalizeRs2ROI();

    // State variables
    bool secondScreenFound;
	bool rs2Opened;
	float lastRs2OpenTry;
    double errorcounts;
	bool ROIcalibrated;
    bool projRs2Calibrated;
//    bool ROIUpdated;
    bool projRs2CalibrationUpdated;
	bool basePlaneComputed;
    bool basePlaneUpdated;
    bool imageStabilized;
    bool waitingForFlattenSand;
    bool drawRs2View;
	bool drawRs2ColorView;
    Calibration_state calibrationState;
    ROI_calibration_state ROICalibState;
    Auto_calibration_state autoCalibState;
    Full_Calibration_state fullCalibState;
	Application_state applicationState;

    // Projector window
    std::shared_ptr<ofAppBaseWindow> projWindow;
    
    //rs2 grabber
    Rs2Grabber               rs2grabber;
    bool                        spatialFiltering;
    bool                        followBigChanges;
    int                         numAveragingSlots;
	bool                        doInpainting;
	bool                        doFullFrameFiltering;

    //rs2 buffer
    ofxCvFloatImage             FilteredDepthImage;
    ofxCvColorImage             rs2ColorImage;
    ofVec2f*                    gradField;
	ofFpsCounter                fpsRs2;
	ofxDatGuiTextInput*         fpsRs2Text;

    // Projector and realsense2 variables
    ofVec2f projRes;
    ofVec2f rs2Res;

    // FBos
    ofFbo fboProjWindow;
    ofFbo fboMainWindow;

    //Images and cv matrixes
    cv::Mat                     cvRgbImage;
	cv::Mat                     cvGrayImage;
//	ofxCvFloatImage             Dptimg;
    
    //Gradient field variables
    int gradFieldcols, gradFieldrows;
    int gradFieldResolution;
    float arrowLength;
    int fishInd;
    
    // Calibration variables
    ofxRs2ProjectorToolkit*  kpt;
    vector<ofVec2f>             currentProjectorPoints;
    vector<cv::Point2f>         cvPoints;
    vector<ofVec3f>             pairsRs2;
    vector<ofVec2f>             pairsProjector;

    // ROI calibration variables
    ofxCvGrayscaleImage         thresholdedImage;
    ofxCvContourFinder          contourFinder;
    float                       threshold;
    ofPolyline                  large;
    ofRectangle                 rs2ROI, rs2ROIManualCalib;
	ofVec2f                     ROIStartPoint;
	ofVec2f                     ROICurrentPoint;
	bool                        doShowROIonProjector;

    // Base plane
    ofVec3f basePlaneNormal, basePlaneNormalBack;
    ofVec3f basePlaneOffset, basePlaneOffsetBack;
    ofVec4f basePlaneEq; // Base plane equation in GLSL-compatible format
    
    // Conversion matrices
    ofMatrix4x4                 rs2ProjMatrix;
    ofMatrix4x4                 rs2WorldMatrix;

    // Max offset for keeping rs2 points
    float maxOffset;
    float maxOffsetSafeRange;
    float maxOffsetBack;
    
    // Autocalib points
    ofPoint* autoCalibPts; // Center of autocalib chess boards
    int currentCalibPts;
    int trials;
    bool upframe;

	// Temporal frame filter for cleaning the colour image used for calibration. It should probably be moved to the grabber class/thread
	CTemporalFrameFilter TemporalFrameFilter;
	// Keeps track of how many frames are acquired since last calibration event
	int TemporalFrameCounter;
	// Type of temporal filtering of colour image 0: Median, 1 :average
	int TemporalFilteringType;

    // Chessboard variables
    int   chessboardSize;
    int   chessboardX;
    int   chessboardY;

    // GUI Modal window & interface
	bool displayGui;
    shared_ptr<ofxModalConfirm>   confirmModal;
    shared_ptr<ofxModalAlert>   calibModal;
    shared_ptr<ofxModalThemeProjRs2>   modalTheme;
    ofxDatGui* gui;
	ofxDatGui* StatusGUI;
	std::string calibrationText;
	
	// Debug functions
	bool DumpDebugFiles;
	std::string DebugFileOutDir;
	std::string GetTimeAndDateString();
	bool savePointPair();
	void SaveFilteredDepthImageDebug();
};


#endif /* defined(__GreatSand__KinectProjector__) */


