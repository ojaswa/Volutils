/***************************************************************************
**                                                                        **
**  Volume Utilities - a set of volume processing utilities               **
**  Copyright (C) 2016-2018 Graphics Research Group, IIIT Delhi           **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Ojaswa Sharma                                        **
**           E-mail: ojaswa@iiitd.ac.in                                   **
**           Date  : 15.01.2019                                           **
****************************************************************************/

#ifndef OCCLUSIONSPECTRUM_H
#define OCCLUSIONSPECTRUM_H

#include <itkImage.h>

enum OcclusionMapType {OcclusionMapLinear, OcclusionMapExponential};//Exponential map only works in FFT (fast) mode.

typedef itk::Image<unsigned char, 3> UCharImageType;

class OcclusionSpectrum
{
public:
    OcclusionSpectrum(UCharImageType::Pointer _img);
    ~OcclusionSpectrum();
    double* getOcclusionSpectrum();
    void computeAlphaChannel(UCharImageType::Pointer alphaImage);

private:
    UCharImageType::Pointer m_image;
    double *m_occlusion;
    int m_radius;

    //Functions
    void computeOcclusionSpectrum(OcclusionMapType type);
    void createSphericalKernel(double *kernel, int kernelSize, OcclusionMapType type);
};

#endif // OCCLUSIONSPECTRUM_H
