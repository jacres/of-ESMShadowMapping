//  testApp.cpp
//
//  Created by James Acres on 12-08-15
//  http://www.jamesacres.com
//  http://github.com/jacres
//  @jimmyacres

#include "testApp.h"

testApp::testApp() :
m_angle(0),
m_bDrawDepth(true),
m_bDrawLight(true),
m_bPaused(false)
{};
    

/*
openFrameworks Exponential Shadow Mapping (ESM) example. This uses OpenGl 2.1 + GLSL 1.20 in order to
as compatible as possible with the way OF works with rendering.
 
main.cpp has a glut display string set: window.setGlutDisplayString("rgb double depth>=32 alpha");
This is needed to request a larger depth buffer than the default. A 16bit depth buffer would probably
speed it up a little, but you'll need to adjust the FBO settings in ofxShadowMapLight.cpp to reflect
that (GL_R32F to GL_R16F). Also, multisampling (samples >= xxx) is not in above string - this will lower
the framerate and prevent blitting the shadow map texture to the screen for debug purposes 
(see note in shadowMapLight::debugShadowMap()

The overall scale of this scene might be a little smaller unit-wise than other OF examples. The reason
for this is that depth buffers are limited in resolution, so it's best to set up your worlds with smaller units
rather than larger (ie. a near/far of 0.1 to 100.0)
*/

//--------------------------------------------------------------
void testApp::setup() {
    ofSetVerticalSync(true);
    
    m_cam.setupPerspective( false, 45.0f, 0.1f, 100.0f );
    m_cam.setDistance(40.0f);
    m_cam.setGlobalPosition( 30.0f, 15.0f, 20.0f );
    
    m_cam.lookAt( ofVec3f( 0.0f, 0.0f, 0.0f ) );
    
    m_shader.load( "shaders/mainScene.vert", "shaders/mainScene.frag" );
    
    setupLights();
    createRandomObjects();
}

//--------------------------------------------------------------
void testApp::update() { 
    ofSetWindowTitle( ofToString( ofGetFrameRate() ) );
}

void testApp::createRandomObjects() {
    
    // create some random boxes
    float bounds = 12.0f;
    
    for ( int i=0; i<400; i++ ) {
        float x = bounds - ofRandomuf()*bounds*2.0f;
        float z = bounds - ofRandomuf()*bounds*2.0f;
        float y = bounds - ofRandomuf()*bounds*2.0f;
        float size = ofRandomuf()*5.0;
        
        m_boxes.push_back( Box( ofVec3f(x, y, z), size ) );
    }
}

void testApp::drawObjects() {
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // floor like plane
    ofPushMatrix();
    ofScale(32.0f, 1.0f, 32.0f);
    ofBox(0,0,0,1.0f);    
    ofPopMatrix();

    // draw our boxes
    vector<Box>::iterator it;
    for ( it=m_boxes.begin() ; it < m_boxes.end(); it++ ) {
       ofBox( it->pos, it->size );
    }
}

void testApp::setupLights() {
    // ofxShadowMapLight extends ofLight - you can use it just like a regular light
    // it's set up as a spotlight, all the shadow work + lighting must be handled in a shader
    // there's an example shader in
    
    // shadow map resolution (must be power of 2), field of view, near, far
    // the larger the shadow map resolution, the better the detail, but slower
    m_shadowLight.setup( 2048, 45.0f, 0.1f, 80.0f );
    m_shadowLight.setBlurLevel(4.0f); // amount we're blurring to soften the shadows
    
    m_shadowLight.setAmbientColor( ofFloatColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
    m_shadowLight.setDiffuseColor( ofFloatColor( 0.9f, 0.9f, 0.9f, 1.0f ) );
    m_shadowLight.setSpecularColor( ofFloatColor( 1.0f, 1.0f, 1.0f, 1.0f ) );
    
    m_shadowLight.setPosition( 10.0f, 10.0f, 45.0f );
    
    ofSetGlobalAmbientColor( ofFloatColor( 0.05f, 0.05f, 0.05f ) );
}

//--------------------------------------------------------------
void testApp::draw() {
    
    glEnable( GL_DEPTH_TEST );
    
    ofDisableAlphaBlending();
    
    if (!m_bPaused) {
        m_angle += 0.25f;
    }
    
    m_shadowLight.lookAt( ofVec3f(0.0,0.0,0.0) );
    m_shadowLight.orbit( m_angle, -30.0, 50.0f, ofVec3f(0.0,0.0,0.0) );

    m_shadowLight.enable();
   
    // render linear depth buffer from light view
    m_shadowLight.beginShadowMap();
        drawObjects();
    m_shadowLight.endShadowMap();
    
    // render final scene
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  
    m_shader.begin();

    m_shadowLight.bindShadowMapTexture(0); // bind shadow map texture to unit 0
    m_shader.setUniform1i("u_ShadowMap", 0); // set uniform to unit 0
    m_shader.setUniform1f("u_LinearDepthConstant", m_shadowLight.getLinearDepthScalar()); // set near/far linear scalar
    m_shader.setUniformMatrix4f("u_ShadowTransMatrix", m_shadowLight.getShadowMatrix(m_cam)); // specify our shadow matrix
    
    m_cam.begin();
    
    m_shadowLight.enable();
        drawObjects();
    m_shadowLight.disable();
    
    if ( m_bDrawLight ) {
        glDisable(GL_CULL_FACE);
        m_shadowLight.draw();
        glEnable(GL_CULL_FACE);
    }
    
    m_cam.end();
    
    m_shadowLight.unbindShadowMapTexture();

    m_shader.end();
    

    // Debug shadowmap
    if ( m_bDrawDepth ) {
        m_shadowLight.debugShadowMap();
    }
    
    // draw info string
    ofDisableLighting();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    ofSetColor(255, 0, 0, 255);
    ofDrawBitmapString("Press SPACE to toggle rendering the shadow map texture (linear depth map)\nPress L to toggle drawing the light\nPress P to toggle pause", ofPoint(15, 20));
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
    
    if ( key == ' ' ) {
        m_bDrawDepth = !m_bDrawDepth;
    } else if ( key == 'l' ) {
        m_bDrawLight = !m_bDrawLight;
    } else if ( key == 'p' ) {
        m_bPaused = !m_bPaused;
    }
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
	
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){
	
}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 
	
}
