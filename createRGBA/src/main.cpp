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

#include <stdio.h>
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkRGBAPixel.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>

#include "occlusionspectrum.h"

using namespace std;

int main(int argc, char **argv)
{
  if(argc < 4) {
    fprintf(stderr, "Usage %s <input gray volume file> <input RGB volume> <output RGBA volume file>\nInput formats supported: MHD, NRRD/NHDR, VTK\n", argv[0]);
    return EXIT_FAILURE;
  }
  char* inputGrayFilename = argv[1];
  char* inputRGBFilename = argv[2];
  char* outputRGBAFilename = argv[3];
  //Perform input check and exit if image is not UCHAR, 3-channel
   //TODO:

  //Read input RGB image
  fprintf(stderr, "Reading input images... ");
  constexpr unsigned int Dimension = 3;
  using ComponentType = unsigned char;
  using UCharImageType = itk::Image<ComponentType, Dimension> ;
  using RGBPixelType = itk::RGBPixel<ComponentType>;
  using RGBImageType = itk::Image<RGBPixelType, Dimension>;

  //Read gray image
  using ReaderTypeGray  = itk::ImageFileReader<UCharImageType>;
  ReaderTypeGray::Pointer readerGray = ReaderTypeGray::New();
  readerGray->SetFileName(inputGrayFilename);
  readerGray->Update();
  UCharImageType::Pointer grayImage = readerGray->GetOutput();
  const UCharImageType::SizeType sizeGray = grayImage->GetLargestPossibleRegion().GetSize();

  //Read RGB image
  using ReaderTypeRGB  = itk::ImageFileReader<RGBImageType>;
  ReaderTypeRGB::Pointer readerRGB = ReaderTypeRGB::New();
  readerRGB->SetFileName(inputRGBFilename);
  readerRGB->Update();
  RGBImageType::Pointer rgbImage = readerRGB->GetOutput();
  const RGBImageType::SizeType sizeRGB = rgbImage->GetLargestPossibleRegion().GetSize();
  if(sizeGray[0] != sizeRGB[0] || sizeGray[1] != sizeRGB[1] || sizeGray[2] != sizeRGB[2])
  {
    fprintf(stderr, "Input volume sized do not match. Exiting...\n");
    return EXIT_FAILURE;
  }
  fprintf(stderr, "done.\n");
  fprintf(stderr, "Volume size: %d x %d x %d\n", sizeRGB[0], sizeRGB[1], sizeRGB[2]);

  //Create alpha channel
  UCharImageType::Pointer alphaImage = UCharImageType::New();
  alphaImage->SetRegions(grayImage->GetLargestPossibleRegion());
  alphaImage->Allocate();
  OcclusionSpectrum *occ = new OcclusionSpectrum(grayImage);
  occ->computeAlphaChannel(alphaImage);

  //Compose alpha channel with RGB to create RGBA image
  using RGBAPixelType = itk::RGBAPixel<ComponentType>;
  using RGBAImageType = itk::Image<RGBAPixelType, Dimension>;
  RGBAImageType::Pointer rgbaImage = RGBAImageType::New();
  rgbaImage->SetRegions(rgbImage->GetLargestPossibleRegion());
  rgbaImage->Allocate();

  itk::ImageRegionIterator<RGBAImageType> rgbaIt(rgbaImage, rgbaImage->GetRequestedRegion());
  itk::ImageRegionConstIterator<RGBImageType> rgbIt(rgbImage, rgbaImage->GetRequestedRegion());
  itk::ImageRegionConstIterator<UCharImageType> alphaIt(alphaImage, alphaImage->GetRequestedRegion());
  rgbIt.GoToBegin();
  alphaIt.GoToBegin();
  rgbaIt.GoToBegin();
  while(!rgbaIt.IsAtEnd())
  {
    itk::RGBAPixel<ComponentType> voxel;
    voxel.SetNthComponent(0, rgbIt.Get().GetRed());
    voxel.SetNthComponent(1, rgbIt.Get().GetGreen());
    voxel.SetNthComponent(2, rgbIt.Get().GetBlue());
    voxel.SetNthComponent(3, alphaIt.Get());
    rgbaIt.Set(voxel);
    ++rgbaIt; ++rgbIt; ++alphaIt;
  }
  fprintf(stderr, "done.\n");

  //Save RGBA image
  fprintf(stderr, "Saving RGBA volume... ");
  using WriterType  = itk::ImageFileWriter<RGBAImageType>;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(outputRGBAFilename);
  writer->SetInput(rgbaImage);
  writer->Update();
  fprintf(stderr, "done.\n");

  return EXIT_SUCCESS;
}
