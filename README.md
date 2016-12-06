[![Build Status](https://travis-ci.org/Inist-CNRS/ghostscript-js.svg?branch=master)](https://travis-ci.org/Inist-CNRS/ghostscript-js)
[![bitHound Overall Score](https://www.bithound.io/github/Inist-CNRS/ghostscript-js/badges/score.svg)](https://www.bithound.io/github/Inist-CNRS/ghostscript-js)

# ghostscript-js

Just a nodeJS wrapper for ghostscript

## Usage
```javascript

const Ghostscript = require('ghostscript-js')

const gs = new Ghostscript

gs.batch()
  .nopause()
  .device()
  .resolution(150)
  .input('/path/to/file.pdf')
  .output('/path/to/file.tif')
  .exec().catch((error) => {
    // Do something
  }).then((sdtout) => {
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
* output - file
* input - file
* exec - Promise (ES6)
* compatibility - number
* pdfsettings - string
