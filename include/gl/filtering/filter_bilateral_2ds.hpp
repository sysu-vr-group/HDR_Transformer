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

#ifndef PIC_GL_FILTERING_FILTER_BILATERAL_2DS_HPP
#define PIC_GL_FILTERING_FILTER_BILATERAL_2DS_HPP

#include "../../gl/filtering/filter.hpp"
#include "../../util/file_lister.hpp"
#include "../../gl/point_samplers/sampler_random_m.hpp"

namespace pic {

enum BF_TYPE {BF_CLASSIC, BF_CROSS, BF_BRUSH};

/**
 * @brief The FilterGLBilateral2DS class
 */
class FilterGLBilateral2DS: public FilterGL
{
protected:
    float sigma_s, sigma_r;
    MRSamplersGL<2> *ms;
    BF_TYPE type;

    //Random numbers tile
    ImageGL *imageRand;
    //Fragment Brush
    std::vector<std::string> fragment_sources;

    void initShaders();
    void FragmentShader();

public:
    /**
     * @brief FilterGLBilateral2DS
     * @param sigma_s
     * @param sigma_r
     * @param type
     */
    FilterGLBilateral2DS(float sigma_s, float sigma_r, BF_TYPE type);

    ~FilterGLBilateral2DS();

    /**
     * @brief update
     * @param sigma_s
     * @param sigma_r
     */
    void update(float sigma_s, float sigma_r);
\
    /**
     * @brief execute
     * @param imgIn
     * @param sigma_s
     * @param sigma_r
     * @return
     */
    static ImageGL *execute(ImageGL *imgIn, float sigma_s, float sigma_r)
    {
        FilterGLBilateral2DS *filter = new FilterGLBilateral2DS(sigma_s, sigma_r,
                BF_CLASSIC);

        ImageGL *imgOut = filter->Process(SingleGL(imgIn), NULL);

        delete filter;
        return imgOut;
    }

    /**
     * @brief execute
     * @param nameFile
     * @param nameOut
     * @param sigma_s
     * @param sigma_r
     * @param testing
     * @return
     */
    static ImageGL *execute(std::string nameFile, std::string nameOut,
                               float sigma_s, float sigma_r, int testing = 1)
    {
        ImageGL imgIn(nameFile);
        imgIn.generateTextureGL(false, GL_TEXTURE_2D);

        FilterGLBilateral2DS *filter = new FilterGLBilateral2DS(sigma_s, sigma_r,
                BF_CLASSIC);

        ImageGL *imgOut = new ImageGL(1, imgIn.width, imgIn.height, imgIn.channels,
                                            IMG_GPU_CPU, GL_TEXTURE_2D);

        GLuint testTQ1;

        if(testing > 1) {
            filter->Process(SingleGL(&imgIn), imgOut);

            testTQ1 = glBeginTimeQuery();

            for(int i = 0; i < testing; i++) {
                filter->Process(SingleGL(&imgIn), imgOut);
            }
        } else {
            testTQ1 = glBeginTimeQuery();
            filter->Process(SingleGL(&imgIn), imgOut);
        }

        GLuint64EXT timeVal = glEndTimeQuery(testTQ1);
        double ms = double(timeVal) / (double(testing) * 1000000.0);
        printf("Stochastic Bilateral Filter on GPU time: %f ms\n", ms);

        std::string nameTime = FileLister::getFileNumber(genBilString("S", sigma_s,
                               sigma_r), "txt");

        FILE *file = fopen(nameTime.c_str(), "w");

        if(file != NULL) {
            fprintf(file, "%f", ms);
            fclose(file);
        }

        //Read from the GPU
        imgOut->loadToMemory();
        imgOut->Write(nameOut);

        return imgOut;
    }

    /**
     * @brief setupAux
     * @param imgIn
     * @param imgOut
     * @return
     */
    ImageGL *setupAux(ImageGLVec imgIn, ImageGL *imgOut)
    {
        imgOut = allocateOutputMemory(imgIn, imgOut, false);

        param.push_back(ms->getImage());
        param.push_back(imageRand);

        return imgOut;
    }
};

PIC_INLINE FilterGLBilateral2DS::FilterGLBilateral2DS(float sigma_s, float sigma_r,
        BF_TYPE type = BF_CLASSIC): FilterGL()
{
    //protected values are assigned/computed
    this->sigma_s = sigma_s;
    this->sigma_r = sigma_r;
    this->type = type;

    //Precomputation of the Gaussian Kernel
    int kernelSize = PrecomputedGaussian::getKernelSize(sigma_s);//,sigma_r);
    int halfKernelSize = kernelSize >> 1;

    //Random numbers
    int nSamplers;

//    if(BF_CLASSIC) {

    imageRand = new ImageGL(1, 128, 128, 1, IMG_CPU, GL_TEXTURE_2D);
    imageRand->setRand();
    imageRand->loadFromMemory();
    *imageRand -= 0.5f;
    nSamplers = 1;

 /*   } else {
    int nRand = 32;
        imageRand = new ImageGL(1, 128, 128, 1, IMG_CPU, GL_TEXTURE_2D);
        imageRand->SetRand();
        *imageRand *= float(nRand - 1);
        imageRand->generateTexture2DU32GL();
        nSamplers = nRand;
    }*/

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

PIC_INLINE FilterGLBilateral2DS::~FilterGLBilateral2DS()
{
    delete imageRand;
    delete ms;

    //free shader etc...
}

PIC_INLINE void FilterGLBilateral2DS::FragmentShader()
{
    std::string fragment_source_classic = MAKE_STRING
                                          (
                                                  uniform sampler2D  u_tex;
                                                  uniform isampler2D u_poisson;
                                                  uniform sampler2D  u_rand;
                                                  uniform int   nSamples;
                                                  uniform float sigma_s_sq_2;
                                                  uniform float sigma_r_sq_2;
                                                  uniform int kernelSize;
                                                  uniform float kernelSizef;
                                                  out     vec4  f_color;

    void main(void) {
        vec3  color = vec3(0.0, 0.0, 0.0);
        ivec2 coordsFrag = ivec2(gl_FragCoord.xy);
        vec3 tmpCol;

        vec3 colRef = texelFetch(u_tex, coordsFrag, 0).xyz;
        float weight = 0.0;

        float shifter = texture(u_rand, gl_FragCoord.xy).x;

        for(int i = 0; i < nSamples; i++) {
            //Coordinates
            ivec3 coords = texelFetch(u_poisson, ivec2(i, shifter), 0).xyz;

            //Texture fetch
            tmpCol = texelFetch(u_tex, coordsFrag.xy + coords.xy, 0).xyz;
            vec3 tmpCol2 = tmpCol - colRef;
            float dstR = dot(tmpCol2.xyz, tmpCol2.xyz);
            int coordsz = coords.x * coords.x + coords.y * coords.y;
            float tmp = exp(-dstR / sigma_r_sq_2 - float(coordsz) / sigma_s_sq_2);
            color.xyz += tmpCol * tmp;
            weight += tmp;
        }

        f_color = vec4(weight > 0 ? (color / weight) : colRef, 1.0);
    }
                                          );

    std::string fragment_source_cross = MAKE_STRING
                                        (
                                            uniform sampler2D	u_tex;
                                            uniform sampler2D	u_edge;
                                            uniform isampler2D	u_poisson;
                                            uniform sampler2D	u_rand;
                                            uniform int			nSamples;
                                            uniform float		sigma_s_sq_2;
                                            uniform float		sigma_r_sq_2;
                                            out     vec4		f_color;

    void main(void) {
        vec3  color = vec3(0.0, 0.0, 0.0);
        ivec2 coordsFrag = ivec2(gl_FragCoord.xy);

        vec3 colRef  = texelFetch(u_tex, coordsFrag, 0).xyz;
        vec3 edgeRef = texelFetch(u_edge, coordsFrag, 0).xyz;

        float weight = 0.0;
        float shifter = texture(u_rand, gl_FragCoord.xy, 0).x;

        for(int i = 0; i < nSamples; i++) {
            //Coordinates
            ivec3 coords = texelFetch(u_poisson, ivec2(i, shifter), 0).xyz;

            //Range difference
            vec3 tmpEdge = texelFetch(u_edge, coordsFrag.xy + coords.xy, 0).xyz;
            vec3 tmpEdge2 = tmpEdge - edgeRef;
            float dstR = dot(tmpEdge2.xyz, tmpEdge2.xyz);
            float tmp = exp(-dstR / sigma_r_sq_2 - float(coords.z) / sigma_s_sq_2);

            //Texture Fetch
            vec3 tmpCol = texelFetch(u_tex, coordsFrag.xy + coords.xy, 0).xyz;
            color.xyz += tmpCol * tmp;
            weight += tmp;
        }

        f_color = vec4(weight > 0.0 ? color / weight : colRef, 1.0);
    }
                                        );

    std::string fragment_source_brush = MAKE_STRING
                                        (
                                            uniform sampler2D  u_tex;
                                            uniform isampler2D u_poisson;
                                            uniform sampler2D  u_rand;
                                            uniform sampler2D  u_mask;
                                            uniform int   nSamples;
                                            uniform float sigma_s_sq_2;
                                            uniform float sigma_r_sq_2;
                                            out     vec4      f_color;

    void main(void) {
        vec3  color = vec3(0.0, 0.0, 0.0);
        ivec2 coordsFrag = ivec2(gl_FragCoord.xy);
        vec3 tmpCol;

        float w = texelFetch(u_mask, coordsFrag, 0).x;
        vec3 colRef = texelFetch(u_tex, coordsFrag, 0).xyz;

        if(w > 0.0f) {
            w = min(w, 1.0f);
            float weight = 0.0;
            float shifter = texture(u_rand, gl_FragCoord.xy, 0).x;

            for(int i = 0; i < nSamples; i++) {
                //Coordinates
                ivec3 coords = texelFetch(u_poisson, ivec2(i, shifter), 0).xyz;
                //Texture fetch
                tmpCol = texelFetch(u_tex, coordsFrag.xy + coords.xy, 0).xyz;
                vec3 tmpCol2 = tmpCol - colRef;
                float dstR = dot(tmpCol2.xyz, tmpCol2.xyz);
                float tmp = exp(-dstR / sigma_r_sq_2 - float(coords.z) / sigma_s_sq_2);
                color.xyz += tmpCol * tmp;
                weight += tmp;
            }

            color = weight > 0.0 ? color / weight : colRef;
            f_color = vec4(color.xyz * w + (1 - w) * colRef.xyz, 1.0);
        } else {
            f_color = vec4(colRef.xyz, 1.0);
        }
    }
                                        );

    fragment_sources.push_back(fragment_source_classic);
    fragment_sources.push_back(fragment_source_cross);
    fragment_sources.push_back(fragment_source_brush);
}

PIC_INLINE void FilterGLBilateral2DS::initShaders()
{
#ifdef PIC_DEBUG
    printf("Number of samples: %d\n", ms->nSamples);
#endif

    int value = -1;

    switch(type) {
    case BF_CLASSIC:
        value = 0;
        break;

    case BF_CROSS:
        value = 1;
        break;

    case BF_BRUSH:
        value = 2;
        break;
    }

    technique.initStandard("330", vertex_source, fragment_sources[value], "FilterGLBilateral2DS");

    update(-1.0f, -1.0f);
}

PIC_INLINE void FilterGLBilateral2DS::update(float sigma_s, float sigma_r)
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

    int kernelSize = PrecomputedGaussian::getKernelSize(this->sigma_s);
    int halfKernelSize = kernelSize >> 1;

    if(flag) {
        Vec2i window = Vec2i(halfKernelSize, halfKernelSize);
        ms->updateGL(window, halfKernelSize);
    }

    //shader update
    float sigma_s_sq_2 = 2.0f * this->sigma_s * this->sigma_s;
    float sigma_r_sq_2 = 2.0f * this->sigma_r * this->sigma_r;

    technique.bind();
    technique.setUniform1i("u_tex", 0);

    if(type == BF_CROSS) {
        technique.setUniform1i("u_edge", 1);
        technique.setUniform1i("u_poisson", 2);
        technique.setUniform1i("u_rand", 3);
        technique.setUniform1i("u_mask", 4);
    } else {
        technique.setUniform1i("u_poisson", 1);
        technique.setUniform1i("u_rand", 2);
        technique.setUniform1i("u_mask", 3);
    }

    technique.setUniform1f("sigma_s_sq_2", sigma_s_sq_2);
    technique.setUniform1f("sigma_r_sq_2", sigma_r_sq_2);
    technique.setUniform1i("kernelSize", kernelSize);
    technique.setUniform1f("kernelSizef", float(kernelSize));
    technique.setUniform1i("nSamples", ms->nSamples >> 1);
    technique.unbind();
}

} // end namespace pic

#endif /* PIC_GL_FILTERING_FILTER_BILATERAL_2DS_HPP */

