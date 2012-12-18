//  ShadowMapLight.cpp
//
//  Created by James Acres on 12-08-15
//  http://www.jamesacres.com
//  http://github.com/jacres
//  @jimmyacres

#include "shadowMapLight.h"

ofVbo ShadowMapLight::s_quadVbo;

// bias matrix to scale -1.0 .. 1.0 to 0 .. 1.0
const ofMatrix4x4 ShadowMapLight::s_biasMat = ofMatrix4x4( 0.5, 0.0, 0.0, 0.0,
                                                             0.0, 0.5, 0.0, 0.0,
                                                             0.0, 0.0, 0.5, 0.0,
                                                             0.5, 0.5, 0.5, 1.0 );

const ofVec2f ShadowMapLight::s_quadVerts[] = {
    ofVec2f( -1.0f, -1.0f ),
    ofVec2f( 1.0f, -1.0f ),
    ofVec2f( 1.0f, 1.0f ),
    ofVec2f( -1.0f, 1.0f )
};

const ofVec2f ShadowMapLight::s_quadTexCoords[] = {
    ofVec2f(0.0f, 0.0f),
    ofVec2f(1.0f, 0.0f),
    ofVec2f(1.0f, 1.0f),
    ofVec2f(0.0f, 1.0f)
};

ShadowMapLight::ShadowMapLight() :
m_shadowMapSize(1024),
m_texelSize(1.0f/1024.0f),
m_blurFactor(4.0f),
m_fbo1Id(0),
m_fbo2Id(0),
m_depthTexture1Id(0),
m_colorTexture1Id(0),
m_colorTexture2Id(0),
m_bIsSetup(false)
{}

void ShadowMapLight::setup( int shadowMapSize, float fov, float near, float far ) {

    // make a projection matrix that we'll use to render a scene from our light's viewpoint
    m_projectionMatrix.makePerspectiveMatrix(fov, shadowMapSize/shadowMapSize, near, far);
    
    m_shadowMapSize = shadowMapSize;
    m_texelSize = 1.0f/shadowMapSize;

    m_viewport = ofRectangle( 0.0f, 0.0f, m_shadowMapSize, m_shadowMapSize );
    
    if ( !m_bIsSetup ) {
        createShadowMapFBO();
        m_blurHShader.load( "shaders/basic.vert", "shaders/gaussblur_h5.frag" );
        m_blurVShader.load( "shaders/basic.vert", "shaders/gaussblur_v5.frag" );
        
        m_bIsSetup = true;
    }
    
    m_linearDepthScalar = 1.0 / (far - near); // this helps us remap depth values to be linear

    m_linearDepthShader.load( "shaders/linearDepthBuffer.vert", "shaders/linearDepthBuffer.frag" );
    m_linearDepthShader.begin();
    m_linearDepthShader.setUniform1f( "u_LinearDepthConstant", m_linearDepthScalar );
    m_linearDepthShader.end();
    
    // full viewport quad vbo
    s_quadVbo.setVertexData( &s_quadVerts[0], 4, GL_STATIC_DRAW );
    s_quadVbo.setTexCoordData( &s_quadTexCoords[0], 4, GL_STATIC_DRAW );
    
    ofLight::setup(); // call ofLight constructor

    setSpotlight(); // configure this light as spotlight
}

void ShadowMapLight::createShadowMapFBO() {
    glActiveTexture(GL_TEXTURE0);
    
    // white border for texture edge clamping
    GLfloat border[] = {1.0f, 1.0f, 1.0f, 1.0f};

    // depth texture
    glGenTextures(1, &m_depthTexture1Id);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture1Id);
    
    // can try changing to GL_NEAREST as well - some hardware will give free blurring with it
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // any objects that end up with tex coords outside of light frustum (0..1) will be white (non-shadowed)
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER ); // clamp to above border color
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
    
    // compare mode that checks if pixel we are writing to texture is less than or equal to existing value.
    // useful when we create a linear depth map since we always want to write pixels that are closer to camera/light (lowest in value)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    
    // luminance is 1 channel (R/red in our case). We only need one since depth is 1 channel
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
    
    // set up the texture
    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_shadowMapSize, m_shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0 );
    glBindTexture(GL_TEXTURE_2D, 0);

    
    // color texture
    glGenTextures(1, &m_colorTexture1Id);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture1Id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // any objects that end up with tex coords outside of light frustum (0..1) will be white (non-shadowed)
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

    // single channel float texture - we'll be writing the linear depth value out to this R32F texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_shadowMapSize, m_shadowMapSize, 0, GL_LUMINANCE, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // create a framebuffer object
    glGenFramebuffers(1, &m_fbo1Id);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo1Id);
    
    // attach textures to FBO depth attachment and color attachment0
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture1Id, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture1Id, 0 );

    // switch back to window-system-provided framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    
	// BLUR FBO - used for ping ponging between m_colorTexture1Id and this one for blurring (horiz and vert passes)
    // create a framebuffer object
    glGenFramebuffers(1, &m_fbo2Id);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo2Id);
    
    // CREATE COLOR TEXTURE
    glGenTextures(1, &m_colorTexture2Id);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture2Id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // any objects that end up with tex coords outside of light frustum (0..1) will be white (non-shadowed)
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_shadowMapSize, m_shadowMapSize, 0, GL_LUMINANCE, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // attach textures to FBO color attachment0 point
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture2Id, 0 );
    
    // unbind/switch back to our window/system framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
    // check FBO status
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(fboStatus != GL_FRAMEBUFFER_COMPLETE)
        printf("GL_FRAMEBUFFER_COMPLETE failed for shadowmap FBO: %u\n", fboStatus );
}

void ShadowMapLight::setBlurLevel(float factor) {
    m_blurFactor = factor;
}

void ShadowMapLight::beginShadowMap() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo1Id); // bind our FBO that has depth and color textures

    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    glEnable(GL_CULL_FACE); // cull front faces - this helps with artifacts and shadows with exponential shadow mapping
    glCullFace(GL_FRONT);
    
    glEnable( GL_DEPTH_TEST );

    m_linearDepthShader.begin();

    ofVec3f eye = getGlobalPosition();
    ofVec3f center = eye + getLookAtDir();
    ofVec3f up = ofVec3f(0.0f, 1.0f, 0.0f);
    
    m_viewMatrix.makeLookAtViewMatrix(eye, center, up); // make our look at view matrix
    
    // load the view and projection matrices + save our current ones so we can restore them when done
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glLoadMatrixf(m_projectionMatrix.getPtr());
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glLoadMatrixf(m_viewMatrix.getPtr());
    
    // save current viewport and set new one to the shadow map dimensions
    glPushAttrib( GL_VIEWPORT_BIT );
    glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
}

void ShadowMapLight::endShadowMap() {
    m_linearDepthShader.end();

    // restore matrices
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    // restore viewport
    glPopAttrib();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    blurShadowMap(); // blur our shadow map
}

ofMatrix4x4 ShadowMapLight::getShadowMatrix( ofCamera &cam ) {
    // create a transform matrix that we can use in our shader to do the following while rendering our objects:
    // - convert our vertex position from camera/view space back to world space (need inverse of the camera's modelviewmatrix)
    // - convert this world space vertex to light clip space (view and projection matrix from out light)
    // - convert this light clip space value from -1.0 .. +1.0 to 0.0 .. 1.0 so that we can use it as a texture lookup for the shadowmap texture
    
    ofMatrix4x4 inverseCameraMatrix = ofMatrix4x4::getInverseOf( cam.getModelViewMatrix() );
    ofMatrix4x4 shadowTransMatrix = inverseCameraMatrix * m_viewMatrix * m_projectionMatrix * s_biasMat;
        
    return shadowTransMatrix;
}

GLuint ShadowMapLight::getColorTextureId() {
    return m_colorTexture1Id;
}

GLuint ShadowMapLight::getDepthTextureId() {
    return m_depthTexture1Id;
}

GLuint ShadowMapLight::getFboId() {
    return m_fbo1Id;
}

float ShadowMapLight::getLinearDepthScalar() {
    return m_linearDepthScalar;
}

void ShadowMapLight::bindShadowMapTexture( int texUnit ) {
    glEnable( GL_TEXTURE_2D );
    glActiveTexture( GL_TEXTURE0 + texUnit );
    glBindTexture(GL_TEXTURE_2D, m_colorTexture1Id);
    
    m_boundTexUnit = texUnit;
}

void ShadowMapLight::unbindShadowMapTexture() {
    if ( m_boundTexUnit != 0 ) {
        glEnable( GL_TEXTURE_2D );
        glActiveTexture( GL_TEXTURE0 + m_boundTexUnit );
        glBindTexture( GL_TEXTURE_2D, 0 );
        glActiveTexture( GL_TEXTURE0 );
        
        m_boundTexUnit = 0;
    }
}

void ShadowMapLight::blurShadowMap() {
    // bind blur FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo2Id);
    // save our old viewport so we can restore it
    glPushAttrib( GL_VIEWPORT_BIT );
    glViewport( 0, 0, m_shadowMapSize, m_shadowMapSize );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); 

    // bind texture containing linear depth buffer
    glEnable( GL_TEXTURE_2D );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture(GL_TEXTURE_2D, m_colorTexture1Id);

    // set identity matrices and save current matrix state
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // horizontal blur pass
    ofSetColor( ofFloatColor(1.0f, 1.0f, 1.0f, 1.0f) );

    m_blurHShader.begin();

    m_blurHShader.setUniform1i( "blurSampler", 0 );
    m_blurHShader.setUniform1f( "sigma", m_blurFactor );
    m_blurHShader.setUniform1f( "blurSize", m_texelSize  );

    // draw the full viewport quad
    s_quadVbo.draw( GL_QUADS, 0, 4 );

    m_blurHShader.end();

    // ping pong fbo back to 1
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo1Id);
    glViewport( 0, 0, m_shadowMapSize, m_shadowMapSize );
    glClear( GL_COLOR_BUFFER_BIT ); 

    // bind horizontally blurred texture
    glBindTexture(GL_TEXTURE_2D, m_colorTexture2Id);

    // vertical blur pass
    m_blurVShader.begin();
    
    m_blurVShader.setUniform1i( "blurSampler", 0 );
    m_blurVShader.setUniform1f( "sigma", m_blurFactor );
    m_blurVShader.setUniform1f( "blurSize", m_texelSize  );
    
    // draw the full viewport quad
    s_quadVbo.draw( GL_QUADS, 0, 4 );
    
    m_blurVShader.end();
    
    // unbind the fbo
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // restore matrices
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib(); // restore our pushed viewport
}

void ShadowMapLight::debugShadowMap() {
    // Blit the frame buffer to the screen - note: this will not work if you have multisampling enabled.
    // You'll have to blit at full size, not a smaller size if this is the case
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo1Id);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, m_shadowMapSize, m_shadowMapSize, 0, 0, 256, 256, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}
