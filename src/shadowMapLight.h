#pragma once

//  ShadowMapLight.h
//
//  Created by James Acres on 12-08-15
//  http://www.jamesacres.com
//  http://github.com/jacres
//  @jimmyacres

#include "ofMain.h"

class ShadowMapLight : public ofLight {
public:	
	ShadowMapLight();
    
    void    setup( int shadowMapSize=1024, float fov=60.0f, float near=0.1f, float far=200.0f );
    void    setBlurLevel( float factor );
    
    void    createShadowMapFBO();
    
    void    beginShadowMap();
    void    endShadowMap();
    
    void    bindShadowMapTexture( int texUnit=0 );
    void    unbindShadowMapTexture();
    
    void    blurShadowMap();
    
    void    debugShadowMap();
    
    // getters
    ofMatrix4x4 getShadowMatrix( ofCamera &cam );
    
    GLuint      getFboId();
    GLuint      getColorTextureId();
    GLuint      getDepthTextureId();
    float       getLinearDepthScalar();
    

protected:

    static const ofMatrix4x4 s_biasMat;
    
    static const ofVec2f  s_quadVerts[];
    static const ofVec2f  s_quadTexCoords[];
    
    static ofVbo    s_quadVbo;

    bool        m_bIsSetup;
    
    GLuint      m_fbo1Id;
    GLuint      m_fbo2Id;
    GLuint      m_depthTexture1Id;
    GLuint      m_colorTexture1Id;
    GLuint      m_colorTexture2Id;

    ofShader    m_blurHShader;
    ofShader    m_blurVShader;
    ofShader    m_linearDepthShader;
    
    ofRectangle m_viewport;
    
    ofMatrix4x4 m_viewMatrix;
    ofMatrix4x4 m_projectionMatrix;

    GLuint      m_boundTexUnit;
        
    float       m_texelSize;
    int         m_shadowMapSize;
    float       m_blurFactor;
    
    float       m_linearDepthScalar;
    
    
};
