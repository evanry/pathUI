#ifndef _MultiResolutionImage
#define _MultiResolutionImage
#include <string>
#include <memory>
#include "multiresolutionimageinterface_export.h"
#include "TileCache.h"
#include "boost/thread.hpp"
#include "core/PathologyEnums.h"
#include "core/ImageSource.h"
#include "core/Patch.h"

class MULTIRESOLUTIONIMAGEINTERFACE_EXPORT MultiResolutionImage : public ImageSource {

public :
  MultiResolutionImage();
  virtual ~MultiResolutionImage();

  //! Load the image, returns whether a valid image is obtained
  virtual bool initialize(const std::string& imagePath) = 0;

  //! Gets/Sets the maximum size of the cache
  virtual const unsigned long long getCacheSize();
  virtual void setCacheSize(const unsigned long long cacheSize);

  //! Gets the number of levels in the slide pyramid
  virtual const int getNumberOfLevels() const;
  
  //! Gets the dimensions of the level 0
  virtual const std::vector<unsigned long long> getDimensions() const;

  //! Gets the dimensions of the specified level of the pyramid
  virtual const std::vector<unsigned long long> getLevelDimensions(const unsigned int& level) const;
  
  //! Get the downsampling factor of the given level relative to the base level
  virtual const double getLevelDownsample(const unsigned int& level) const;

  //! Gets the level corresponding to the closest downsample factor given the requested downsample factor
  virtual const int getBestLevelForDownSample(const double& downsample) const;

  //! Gets the minimum value for a channel. If no channel is specified, default to the first channel
  virtual double getMinValue(int channel = -1) = 0;
  
  //! Gets the maximum value for a channel. If no channel is specified, default to the first channel
  virtual double getMaxValue(int channel = -1) = 0;

  //! Obtains data as a patch, which is a basic image class containing all relevant information for further processing,
  //! like data and colortype
  template <typename T> 
  Patch<T> getPatch(const long long& startX, const long long& startY, const unsigned long long& width,
    const unsigned long long& height, const unsigned int& level) 
  {
    std::vector<unsigned long long> dims(3,0);
    dims[0] = width;
    dims[1] = height;
    dims[2] = _samplesPerPixel;
    T* data = new T[width*height*_samplesPerPixel];
    getRawRegion<T>(startX, startY, width, height, level, data);
    std::vector<double> patchSpacing(_spacing.size(), 1.0);
    double levelDownsample = this->getLevelDownsample(level);
    for (unsigned int i = 0; i < _spacing.size(); ++i) {
      patchSpacing[i] = _spacing[i] * levelDownsample;
    }
    Patch<T> patch = Patch<T>(dims, this->getColorType(), data, true);
    patch.setSpacing(patchSpacing);
    return patch;
  }

  //! Obtains pixel data for a requested region. The user is responsible for allocating
  //! enough memory for the data to fit the array and clearing the memory. Please note that in case of int32 ARGB data,  
  //! like in OpenSlide, the order of the colors depends on the endianness of your machine (Windows typically BGRA)
  template <typename T> 
  void getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width, 
    const unsigned long long& height, const unsigned int& level, T*& data) {
      if (level >= getNumberOfLevels()) {
        return;
      }
      unsigned int nrSamples = getSamplesPerPixel();
      if (this->getDataType()==pathology::Float) {
        float * temp = (float*)readDataFromImage(startX, startY, width, height, level);
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
      else if (this->getDataType()==pathology::UChar) {
        unsigned char * temp = (unsigned char*)readDataFromImage(startX, startY, width, height, level);
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
      else if (this->getDataType()==pathology::UInt16) {
        unsigned short * temp = (unsigned short*)readDataFromImage(startX, startY, width, height, level);
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
      else if (this->getDataType()==pathology::UInt32) {
        unsigned int * temp = (unsigned int*)readDataFromImage(startX, startY, width, height, level);
        std::copy(temp, temp + width*height*nrSamples, data);
        delete[] temp;
      }
    }

protected :

  //! To make MultiResolutionImage thread-safe
  boost::shared_mutex _openCloseMutex;
  boost::mutex _cacheMutex;
  std::shared_ptr<void> _cache;

  // Aditional properties of a multi-resolution image
  std::vector<std::vector<unsigned long long> > _levelDimensions;
  unsigned int _numberOfLevels;

  // Properties of the loaded slide
  unsigned long long _cacheSize;
  std::string _fileType;

  // Cleans up internals
  virtual void cleanup();

  // Reads the actual data from the image
  virtual void* readDataFromImage(const long long& startX, const long long& startY, const unsigned long long& width, 
    const unsigned long long& height, const unsigned int& level) = 0;

  template <typename T> void createCache() {
    if (_isValid) {
      _cache.reset(new TileCache<T>(_cacheSize));
    }
  }
};

template <> void MULTIRESOLUTIONIMAGEINTERFACE_EXPORT MultiResolutionImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, const unsigned int& level, unsigned char*& data);

template <> void MULTIRESOLUTIONIMAGEINTERFACE_EXPORT MultiResolutionImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, const unsigned int& level, unsigned short*& data);

template <> void MULTIRESOLUTIONIMAGEINTERFACE_EXPORT MultiResolutionImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, const unsigned int& level, unsigned int*& data);

template <> void MULTIRESOLUTIONIMAGEINTERFACE_EXPORT MultiResolutionImage::getRawRegion(const long long& startX, const long long& startY, const unsigned long long& width,
  const unsigned long long& height, const unsigned int& level, float*& data);

#endif