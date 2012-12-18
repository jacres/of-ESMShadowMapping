//  testApp.h
//
//  Created by James Acres on 12-08-15
//  http://www.jamesacres.com
//  http://github.com/jacres
//  @jimmyacres

#pragma once

#include "ofMain.h"
#include "shadowMapLight.h"

class testApp : public ofBaseApp {
    
    struct Box {
        ofVec3f pos;
        float size;
        
        Box(ofVec3f pos=ofVec3f(0.0f, 0.0f, 0.0f), float size=2.0f) :
            pos(pos),
            size(size)
        {}
    };
    
	public:
        testApp();
		
        void setup();
		void update();
		void draw();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
        void setupLights();
        void createRandomObjects();
        void drawObjects();
    
        ofEasyCam m_cam;
        ShadowMapLight m_shadowLight;
    
        ofShader m_shader;
    
        float   m_angle;    
        bool    m_bDrawDepth;
        bool    m_bDrawLight;
        bool    m_bPaused;

        vector<Box> m_boxes;
};
