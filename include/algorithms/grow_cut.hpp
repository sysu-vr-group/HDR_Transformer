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

#ifndef PIC_ALGORITHMS_GROW_CUT_HPP
#define PIC_ALGORITHMS_GROW_CUT_HPP

#include "../base.hpp"

#include "../image.hpp"
#include "../filtering/filter_max.hpp"
#include "../filtering/filter_grow_cut.hpp"
#include "../filtering/filter_channel.hpp"

namespace pic {

class GrowCut
{
protected:
    FilterGrowCut flt;

public:

    /**
     * @brief GrowCut
     */
    GrowCut()
    {

    }

    /**
     * @brief fromStrokeImageToSeeds
     * @param strokes
     * @param out
     * @return
     */
    static Image *fromStrokeImageToSeeds(Image *strokes, Image *out)
    {
        if(strokes->channels < 3) {
            return out;
        }

        if(out == NULL) {
            out = new Image(1, strokes->width, strokes->height, 1);
        }

        //red  --> +1
        //blue --> -1
        float red[] = {1.0f, 0.0f, 0.0f};
        float blue[] = {0.0f, 0.0f, 1.0f};

        for(int i = 0; i < strokes->nPixels(); i++) {
            int ind = i * strokes->channels;

            float d_red  = sqrtf(Arrayf::distanceSq(red,  &strokes->data[ind], 3));
            float d_blue = sqrtf(Arrayf::distanceSq(blue, &strokes->data[ind], 3));

            out->data[i] = 0.0f;

            out->data[i] = d_red  < 0.5f ?  1.0f : out->data[i];
            out->data[i] = d_blue < 0.5f ? -1.0f : out->data[i];
        }

        return out;
    }

    /**
     * @brief getMaskAsImage
     * @param state
     * @return
     */
    static Image* getMaskAsImage(Image *state, Image *out)
    {
        if(state == NULL) {
            return out;
        }

        if(out == NULL) {
            out = new Image(1, state->width, state->height, 1);
        }

        return FilterChannel::execute(state, out, 0);
    }

    /**
     * @brief execute
     * @param img
     * @param seeds
     * @param state_cur
     * @return
     */
    Image *execute(Image *img, Image *seeds, Image *state_cur = NULL)
    {
        if(img == NULL || seeds == NULL) {
            return NULL;
        }

        if(state_cur == NULL) {
            state_cur = new Image(img->width, img->height, 2);
        }

        Image *state_next = state_cur->allocateSimilarOne();

        //compute max
        Image *img_max = FilterMax::execute(img, NULL, 5);

        for(int i = 0; i < state_cur->nPixels(); i++) {
            //init state_cur
            int j  = i * state_cur->channels;
            int j2 = i * seeds->channels;
            state_cur->data[j] = seeds->data[j2];
            state_cur->data[j + 1] = fabsf(seeds->data[j2]) > 0.0f ? 1.0f : 0.0f;

            //fix max
            j = i * img_max->channels;
            img_max->data[j] = Arrayf::norm_sq(&img_max->data[j], img_max->channels);
        }

        //iterative filtering...
        int iterations = int(img->getDiagonalSize());

        if((iterations % 2) == 1) {
            iterations++;
        }

        ImageVec input = Triple(state_cur, img, img_max);
        Image *output = state_next;

        for(int i = 0; i < iterations; i++) {
            output = flt.Process(input, output);

            Image *tmp = input[0];
            input[0] = output;
            output = tmp;
        }

        delete output;
        delete img_max;

        return input[0];
    }

};

} // end namespace pic

#endif /* PIC_ALGORITHMS_GROW_CUT_HPP */

