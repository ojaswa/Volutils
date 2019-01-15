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
#include <itkRGBToLuminanceImageFilter.h>
#include <itkRGBAPixel.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>

#include "occlusionspectrum.h"

using namespace std;

int main(int argc, char **argv)
{
  if(argc < 3) {
    fprintf(stderr, "Usage %s <input volume file> <output volume file>\nInput formats supported: MHD, NRRD/NHDR, VTK\n", argv[0]);
    return EXIT_FAILURE;
  }
  char* inputFilename = argv[1];
  char* outputFilename = argv[2];
  //Perform input check and exit if image is not UCHAR, 3-channel
   //TODO:

  //Read input RGB image
  fprintf(stderr, "Reading input image... ");
  constexpr unsigned int Dimension = 3;
  using ComponentType = unsigned char;
  using RGBPixelType = itk::RGBPixel<ComponentType>;
  using RGBImageType = itk::Image<RGBPixelType, Dimension>;

  using ReaderType  = itk::ImageFileReader<RGBImageType>;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(inputFilename);
  reader->Update();
  RGBImageType::Pointer rgbImage = reader->GetOutput();
  const RGBImageType::SizeType size = rgbImage->GetLargestPossibleRegion().GetSize();
  fprintf(stderr, "done.\n");
  fprintf(stderr, "Volume size: %d x %d x %d\n", size[0], size[1], size[2]);

  //Convert 3-channel image to grayscale volume
  fprintf(stderr, "Generating alpha channel... ");
  using UCharImageType = itk::Image<ComponentType, Dimension> ;
  using RGB2grayFilterType = itk::RGBToLuminanceImageFilter<RGBImageType, UCharImageType>;
  RGB2grayFilterType::Pointer rgb2gray = RGB2grayFilterType::New();
  rgb2gray->SetInput(rgbImage);

  //Create alpha channel
  UCharImageType::Pointer alphaImage = UCharImageType::New();
  alphaImage->SetRegions(rgbImage->GetLargestPossibleRegion());
  alphaImage->Allocate();
  OcclusionSpectrum *occ = new OcclusionSpectrum(rgb2gray->GetOutput());
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
  writer->SetFileName(outputFilename);
  writer->SetInput(rgbaImage);
  writer->Update();
  fprintf(stderr, "done.\n");

  return EXIT_SUCCESS;
}
