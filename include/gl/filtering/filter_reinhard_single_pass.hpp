/*

PICCANTE
The hottest HDR imaging library!
http://vcg.isti.cnr.it/piccante

Copyright (C) 2014
Visual Computing Laboratory - ISTI CNR
http://vcg.isti.cnr.it
First author: Francesco Banterle

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef PIC_GL_FILTERING_REINHARD_TMO_SINGLE_PASS_HPP
#define PIC_GL_FILTERING_REINHARD_TMO_SINGLE_PASS_HPP

#include "../../base.hpp"

#include "../../util/vec.hpp"

#include "../../gl/filtering/filter.hpp"
#include "../../util/file_lister.hpp"
#include "../../gl/point_samplers/sampler_random_m.hpp"

namespace pic {

/**
 * @brief The FilterGLReinhardSinglePass class
 */
class FilterGLReinhardSinglePass: public FilterGL
{
protected:
    float sigma_s, sigma_r, sigmoid_constant;
    MRSamplersGL<2> *ms;

    //tmo
    float alpha;

    //Random numbers tile
    ImageGL *imageRand;

    void initShaders();
    void FragmentShader();

public:
    float Lwa;
    /**
     * @brief FilterGLReinhardSinglePass
     * @param sigma_s
     * @param sigma_r
     * @param type
     */
    FilterGLReinhardSinglePass(float alpha, float phi);

    ~FilterGLReinhardSinglePass();

    /**
     * @brief update
     * @param sigma_s
     * @param sigma_r
     */
    void update(float sigma_s, float sigma_r, float Lwa);

    /**
     * @brief Process
     * @param imgIn
     * @param imgOut
     * @return
     */
    ImageGL *Process(ImageGLVec imgIn, ImageGL *imgOut);
\

};

PIC_INLINE FilterGLReinhardSinglePass::FilterGLReinhardSinglePass(float alpha, float phi = 8.0f): FilterGL()
{
    this->alpha = alpha;

    float epsilon = 0.05f;
    float s_max = 8.0f;
    float sigma_s = 1.6f;
    float sigma_r = epsilon / 2.0f;

    this->sigmoid_constant = (powf(2.0f, phi) * alpha / (s_max * s_max)) * epsilon;

    //protected values are assigned/computed
    this->sigma_s = sigma_s;
    this->sigma_r = sigma_r;

    //Precomputation of the Gaussian Kernel
    int kernelSize = PrecomputedGaussian::getKernelSize(this->sigma_s);//,sigma_r);
    int halfKernelSize = kernelSize >> 1;

    //Random numbers
    int nSamplers;

    imageRand = new ImageGL(1, 128, 128, 1, IMG_CPU, GL_TEXTURE_2D);
    imageRand->setRand();
    imageRand->loadFromMemory();
    *imageRand -= 0.5f;
    nSamplers = 1;


    //Poisson samples
#ifdef PIC_DEBUG
    printf("Window: %d\n", halfKernelSize);
#endif

    Vec2i window = Vec2i(halfKernelSize, halfKernelSize);
    ms = new MRSamplersGL<2>(ST_BRIDSON, window, halfKernelSize, 1,
                             nSamplers);
    ms->generateTexture();

    FragmentShader();
    initShaders();
}

PIC_INLINE FilterGLReinhardSinglePass::~FilterGLReinhardSinglePass()
{
    delete imageRand;
    delete ms;

    //free shader etc...
}

PIC_INLINE void FilterGLReinhardSinglePass::FragmentShader()
{
    fragment_source = MAKE_STRING
                                          (
                                                  uniform sampler2D  u_tex;
                                                  uniform sampler2D  u_tex_col;
                                                  uniform isampler2D u_poisson;
                                                  uniform sampler2D  u_rand;
                                                  uniform int   nSamples;
                                                  uniform float sigmoid_constant;
                                                  uniform float sigmas2;
                                                  uniform float sigmar2;
                                                  uniform int kernelSize;
                                                  uniform float kernelSizef;
                                                  uniform float a;
                                                  out     vec4  f_color;

    void main(void) {
        ivec2 coordsFrag = ivec2(gl_FragCoord.xy);

        float colRef = texelFetch(u_tex, coordsFrag, 0).x;
        float Lw = colRef;

        colRef = colRef / (colRef + sigmoid_constant);


        float shifter = texture(u_rand, gl_FragCoord.xy).x;
        float  color = 0.0;
        float weight = 0.0;

        for(int i = 0; i < nSamples; i++) {
            //Coordinates
            ivec3 coords = texelFetch(u_poisson, ivec2(i, shifter), 0).xyz;

            //Texture fetch
            float tmpCol = texelFetch(u_tex, coordsFrag.xy + coords.xy, 0).x;
            tmpCol = tmpCol / (tmpCol + sigmoid_constant);

            float tmpCol2 = tmpCol - colRef;
            float dstR = tmpCol2 * tmpCol2;

            int coordsz = coords.x * coords.x + coords.y * coords.y;
            float tmp = exp(-dstR / sigmar2 - float(coordsz) / sigmas2);

            color += tmpCol * tmp;
            weight += tmp;

        }

        float bilateral = weight > 0.0 ? (color / weight) : colRef;
        bilateral = (bilateral * sigmoid_constant) / (1.0 - bilateral);

        Lw = Lw < 1e-9 ? 1e-9 : Lw;
        vec3 color_hdr = texelFetch(u_tex_col, coordsFrag, 0).xyz / Lw;

        float Ld = (Lw * a) / (bilateral * a + 1.0);

        f_color = vec4(color_hdr * Ld, 1.0);
    }
                                          );

}

PIC_INLINE void FilterGLReinhardSinglePass::initShaders()
{
#ifdef PIC_DEBUG
    printf("Number of samples: %d\n", ms->nSamples);
#endif

    technique.initStandard("330", vertex_source, fragment_source, "FilterGLReinhardSinglePass");

    update(-1.0f, -1.0f, 1.0f);
}

PIC_INLINE void FilterGLReinhardSinglePass::update(float sigma_s, float sigma_r, float Lwa)
{
    bool flag = false;

    if(sigma_s > 0.0f) {
        flag = (this->sigma_s == sigma_s);
        this->sigma_s = sigma_s;
    }

    if(sigma_r > 0.0f) {
        flag = flag || (this->sigma_r == sigma_r);
        this->sigma_r = sigma_r;
    }

    if(Lwa > 0.0f) {
        this->Lwa = Lwa;
    }

    int kernelSize = PrecomputedGaussian::getKernelSize(this->sigma_s);
    int halfKernelSize = kernelSize >> 1;

    if(flag) {
        Vec2i window = Vec2i(halfKernelSize, halfKernelSize);
        ms->updateGL(window, halfKernelSize);
    }

    //shader update
    float sigmas2 = 2.0f * this->sigma_s * this->sigma_s;
    float sigmar2 = 2.0f * this->sigma_r * this->sigma_r;

    technique.bind();
    technique.setUniform1i("u_tex",       0);
    technique.setUniform1i("u_poisson",   1);
    technique.setUniform1i("u_rand",      2);
    technique.setUniform1i("u_tex_col",   3);

    technique.setUniform1f("sigmas2",         sigmas2);
    technique.setUniform1f("a",               alpha / Lwa);
    technique.setUniform1f("sigmoid_constant", sigmoid_constant);

    technique.setUniform1f("sigmar2",         sigmar2);
    technique.setUniform1i("kernelSize",      kernelSize);
    technique.setUniform1f("kernelSizef",     float(kernelSize));
    technique.setUniform1i("nSamples",        ms->nSamples >> 1);
    technique.unbind();
}

PIC_INLINE ImageGL *FilterGLReinhardSinglePass::Process(ImageGLVec imgIn,
        ImageGL *imgOut)
{
    if(imgIn[0] == NULL) {
        return imgOut;
    }

    int w = imgIn[0]->width;
    int h = imgIn[0]->height;

    //TODO: check if other have height and frames swapped
    if(imgOut == NULL) {
        imgOut = new ImageGL(imgIn[0]->frames, w, h, imgIn[0]->channels, IMG_GPU, GL_TEXTURE_2D);
    }

    if(fbo == NULL) {
        fbo = new Fbo();
    }

    fbo->create(w, h, imgIn[0]->frames, false, imgOut->getTexture());

    ImageGL *edge, *base;

    edge = imgIn[0];
    base = imgIn[1];

    //Rendering
    fbo->bind();
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    //Shaders
    technique.bind();

    //Textures
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, edge->getTexture());

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, imageRand->getTexture());

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ms->getTexture());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, base->getTexture());

    //Rendering aligned quad
    quad->Render();

    //Fbo
    fbo->unbind();

    //Shaders
    technique.unbind();

    //Textures
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return imgOut;
}

} // end namespace pic

#endif /* PIC_GL_FILTERING_REINHARD_TMO_SINGLE_PASS_HPP */

