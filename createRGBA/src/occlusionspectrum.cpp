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

#include "occlusionspectrum.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <float.h>
#include <itkImage.h>
#include <itkFFTConvolutionImageFilter.h>
#include <itkImportImageFilter.h>
#include <itkCastImageFilter.h>

OcclusionSpectrum::OcclusionSpectrum(UCharImageType::Pointer _img)
{
    m_image = _img;
    m_occlusion = NULL;

    const UCharImageType::SizeType size = m_image->GetLargestPossibleRegion().GetSize();
    m_radius = 0.1 * (size[0] + size[1] + size[2]) / 3;//Radius should capture the object sizes in the volume.
}

// Use Fast Fourier Transform to compute convolution
void OcclusionSpectrum::computeOcclusionSpectrum(OcclusionMapType type)
{
    // Create a spherical kernel
    int kernelSize = m_radius*2 + 1;
    typedef itk::Image<double, 3> DoubleImageType;
    typedef itk::FFTConvolutionImageFilter<DoubleImageType> FilterType;
    typedef itk::ImportImageFilter<double, 3> ImportFilterType;

    // Create kernel data
    DoubleImageType::Pointer kernel = DoubleImageType::New();
    DoubleImageType::SizeType imsize;
    imsize[0] = kernelSize; imsize[1] = kernelSize; imsize[2] = kernelSize;
    ImportFilterType::IndexType start;
    start.Fill(0);
    ImportFilterType::RegionType region;
    region.SetIndex(start);
    region.SetSize(imsize);
    kernel->SetRegions(region);
    kernel->Allocate();
    kernel->GetPixelContainer()->SetContainerManageMemory(true);
    double *ptr_kernel = reinterpret_cast<double*>(kernel->GetPixelContainer()->GetImportPointer());
    createSphericalKernel(ptr_kernel, kernelSize, type);

    //Cast image to double
    using CastFilterType = itk::CastImageFilter<UCharImageType, DoubleImageType>;
    CastFilterType::Pointer castFilter = CastFilterType::New();
    castFilter->SetInput(m_image);

    // Create convolution kernel
    FilterType::Pointer convolutionFilter = FilterType::New();
    convolutionFilter->SetInput(castFilter->GetOutput());
    // Set kernel
    convolutionFilter->SetKernelImage(kernel);
    convolutionFilter->Update();

    // Get data from convolution filter
    convolutionFilter->GetOutput()->GetPixelContainer()->SetContainerManageMemory(false);
    m_occlusion = reinterpret_cast<double*>(convolutionFilter->GetOutput()->GetPixelContainer()->GetImportPointer());
    convolutionFilter->Update();      
}

void OcclusionSpectrum::createSphericalKernel(double *ptr_kernel, int kernelSize, OcclusionMapType mapType)
{
    int kernelElems = 0;
    int r2 = m_radius*m_radius;
    int ii, jj, kk;
    int dist2, pred;
    for(int k=0; k<kernelSize; k++)
        for(int j=0; j<kernelSize; j++)
            for(int i=0; i<kernelSize; i++) {
                ii = (i - m_radius);
                jj = (j - m_radius);
                kk = (k - m_radius);
                dist2 = ii*ii + jj*jj  + kk*kk;
                pred = r2 - dist2;
                if (pred < 0)
                    ptr_kernel[i + kernelSize*(j + k*kernelSize)] = 0.0; // Outside sphere
                else {
                    ptr_kernel[i + kernelSize*(j + k*kernelSize)] = (mapType == OcclusionMapExponential)? exp(-dist2) : 1.0; //Inside Sphere
                    kernelElems++;
                }
            }
    int kernelSize3 = kernelSize*kernelSize*kernelSize;
    for(int i=0; i<kernelSize3; i++) ptr_kernel[i] /= (double)kernelElems;
}

void OcclusionSpectrum::computeAlphaChannel(UCharImageType::Pointer alphaImage)
{
    //Compute occlusion spectrum
    computeOcclusionSpectrum(OcclusionMapLinear);

    //Normalize to 8-bit and package as alpha channel
    double occ_min = DBL_MAX, occ_max = DBL_MIN;
    const UCharImageType::SizeType size = m_image->GetLargestPossibleRegion().GetSize();
    unsigned long nelem  = size[0]*size[1]*size[2];
    for(unsigned long i=0; i<nelem; i++) {
        if(m_occlusion[i] < occ_min) occ_min = m_occlusion[i];
        if(m_occlusion[i] > occ_max) occ_max = m_occlusion[i];
    }
    fprintf(stderr, "Occlusion: (Min = %f, Max = %f) ", occ_min, occ_max);

    itk::ImageRegionIterator<UCharImageType> alphaIt(alphaImage, alphaImage->GetRequestedRegion());
    alphaIt.GoToBegin();
    unsigned long i = 0;
    unsigned char val;
    double factor = 255.0/(occ_max - occ_min);
    while(!alphaIt.IsAtEnd()) {
        val = (unsigned char)((m_occlusion[i++] - occ_min)*factor);
        alphaIt.Set(val);
        ++alphaIt;
    }
}
