/*

PICCANTE
The hottest HDR imaging library!
http://vcg.isti.cnr.it/piccante

copyright (C) 2014
Visual Computing Laboratory - ISTI CNR
http://vcg.isti.cnr.it
First author: Francesco Banterle

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef PIC_UTIL_RAW_HPP
#define PIC_UTIL_RAW_HPP

#include "../base.hpp"
#include "../util/file_lister.hpp"
#include "../util/string.hpp"

namespace pic {

enum RAW_type { RAW_U16_RGGB, RAW_U8_RGGB, RAW_DOUBLE, RAW_FLOAT};

template <class T> class RAW
{
public:
    T			*data;
    int			nData;
    bool		valid;

    /**
     * @brief RAW
     */
    RAW();

    /**
     * @brief RAW
     * @param nData
     */
    RAW(int nData)
    {
        data = NULL;
        allocate(nData);
    }

    /**
     * @brief RAW
     * @param nameFile
     * @param nData
     */
    RAW(std::string nameFile, int nData = -1)
    {
        valid = false;
        data = NULL;
        nData = 0;

        Read(nameFile, nData);
    }

    ~RAW();

    /**
     * @brief allocate
     * @param nData
     * @return
     */
    bool allocate(int nData)
    {
        if(nData < 1) {
            return false;
        }

        if(data != NULL) {
            delete[] data;
        }

        this->nData = nData;
        data = new T[nData];

        return true;
    }

    /**
     * @brief Read
     * @param nameFile
     * @param nData
     * @return
     */
    bool Read(std::string nameFile, int nData)
    {
        std::ifstream file;
        file.open(nameFile.c_str(), std::ios::in | std::ios::binary);

        if(!file.is_open()) {
            return false;
        }

        //Calculate length of the file
        file.seekg(0, std::ios::end);
        std::ios::streampos length = file.tellg();
        file.seekg(0, std::ios::beg);

        if(nData < 1) {
            nData = length / sizeof(T);
        }

        bool bRet = allocate(nData);


        file.read((char *)data, nData * sizeof(T) / sizeof(char));
        file.close();

        valid = true;
        return bRet;

    }

    /**
     * @brief Write
     * @param nameFile
     * @return
     */
    bool Write(std::string nameFile);

    /**
     * @brief copy
     * @return
     */
    RAW<T> *copy();

    /**
     * @brief sub
     * @param b
     */
    void sub(RAW<T> b);

    /**
     * @brief release
     */
    void release();
};

template <class T> PIC_INLINE RAW<T>::RAW()
{
    valid = false;
    data = NULL;
    nData = 0;
}

template <class T> PIC_INLINE RAW<T>::~RAW()
{
    release();
}

template <class T> PIC_INLINE RAW<T> *RAW<T>::copy()
{
    RAW<T> *out = new RAW<T>;
    out->data = new T[nData];
    out->valid = valid;
    out->nData = nData;
    return out;
}

template <class T> PIC_INLINE bool RAW<T>::Write(std::string nameFile)
{
    std::ofstream file;
    file.open(nameFile.c_str(), std::ios::binary);

    if(file.is_open()) {
        file.write((char *)data, nData * sizeof(T) / sizeof(char));
        file.close();
        valid = true;
        return true;
    } else {
        return false;
    }
}

template <class T> PIC_INLINE void RAW<T>::sub(RAW<T> b)
{

    for(int i = 0; i < nData; i++) {
        int tmp = data[i] - b.data[i];
        data[i] = tmp >= 0 ? tmp : 0;
    }
}

template <class T> PIC_INLINE void RAW<T>::release()
{
    if(data != NULL) {
        data = NULL;
    }

    //	delete[] data;
    valid = false;
    nData = 0;
}

template <class T> PIC_INLINE RAW<T> *getMeanRAWStack(std::vector<RAW<T> > &stack)
{
    if(stack.size() <= 0) {
        return NULL;
    }

    int nData = stack[0].nData;
    RAW<T> *dataOut = new RAW<T>();
    dataOut->data = new T[nData];
    dataOut->nData = nData;

    int stackSize = stack.size();

    for(int i = 0; i < nData; i++) {
        unsigned long tmpData = 0;

        for(int j = 0; j < stackSize; j++) {
            tmpData += stack[j].data[i];
        }

        dataOut->data[i] = tmpData / stackSize;
    }

    dataOut->valid = true;
    return dataOut;
}

template <class T> PIC_INLINE unsigned long *getMeanRAWIterative(RAW<T> *img,
        unsigned long *dataAcc, bool bStart)
{

    if(dataAcc == NULL) {
        dataAcc = new unsigned long[img->nData];
    }

    if(bStart) {
        for(int i = 0; i < img->nData; i++) {
            dataAcc[i] = img->data[i];
        }
    } else {
        for(int i = 0; i < img->nData; i++) {
            dataAcc[i] += img->data[i];
        }
    }

    return dataAcc;
}

template <class T> PIC_INLINE RAW<T> *getMeanRAWFromFile(
    std::string nameDir,
    std::string nameFilter,
    int width,
    int height)
{

    StringVec vec;

    FileLister::getList(nameDir, nameFilter, &vec);

    RAW<T> imgRAW;
    unsigned long *dataAcc = NULL;

    for(unsigned int i = 0; i < vec.size(); i++) {
        imgRAW.Read(vec[i], width * height);
        dataAcc = getMeanRAWIterative<T>(&imgRAW, dataAcc, i == 0);
    }

    RAW<T> *imgOut = imgRAW.copy();

    for(int i = 0; i < imgOut->nData; i++) {
        imgOut->data[i] = dataAcc[i] / vec.size();
    }

    return imgOut;
}

template <class T> PIC_INLINE void getMeanRAWFromFile(
    std::string nameDir,
    std::string nameFilter,
    std::string nameOut,
    int width,
    int height)
{
    (getMeanRAWFromFile<T>(nameDir, nameFilter, width,
                                 height))->Write(nameOut);
}

} // end namespace pic

#endif /* PIC_UTIL_RAW_HPP */

