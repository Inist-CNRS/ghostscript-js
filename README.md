# ghostscript-js

Just a nodeJS wrapper for ghostscript

## Usage
```javascript

const Ghostscript = require('ghostscript-js')

let gs = new Ghostscript

gs
  .batch()
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
* device - device - defaults to tiff24nc
* resolution - number
* firstPage - number
* lastPage - number
* output - file
* input - file
* exec - Promise (ES6)
