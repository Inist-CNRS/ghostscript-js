[![Build Status](https://travis-ci.org/Inist-CNRS/ghostscript-js.svg?branch=master)](https://travis-ci.org/Inist-CNRS/ghostscript-js)
[![bitHound Overall Score](https://www.bithound.io/github/Inist-CNRS/ghostscript-js/badges/score.svg)](https://www.bithound.io/github/Inist-CNRS/ghostscript-js)
[![js-semistandard-style](https://img.shields.io/badge/code%20style-semistandard-brightgreen.svg?style=flat-square)](https://github.com/Flet/semistandard)

# ghostscript-js

Just a nodeJS wrapper for ghostscript

## Usage
```javascript

const Ghostscript = require('ghostscript-js')

const gs = new Ghostscript()

gs.batch()
  .nopause()
  .device()
  .resolution(150)
  .input('/path/to/file.pdf')
  .output('/path/to/file.tif')
  .exec()
  .then((sdtout) => {
    // Do something
  })
  .catch((error) => {
    // Do something
  })
```

## API

* batch
* nopause
* quiet
* interpolate
* ram - number - defaults to 30 MB
* device - device - defaults to tiff24nc
* resolution - number
* firstPage - number
* lastPage - number
* AutoRotatePages - All, None, PageByPage
* antiAliasColorImage
* antiAliasGrayImage
* antiAliasMonoImage
* autoFilterColorImages
* colorImageFilter
* autoFilterGrayImages
* grayImageFilter
* downsampleColorImages
* downsampleGrayImages
* downsampleMonoImages
* colorConversionStrategy
* convertCMYKImagesToRGB
* convertImagesToIndexed
* UCRandBGInfo
* preserveHalftoneInfo
* preserveOPIComments
* preserveOverprintSettings
* output - file
* input - file
* exec - Promise (ES6)
* compatibility - number
* pdfsettings - string
