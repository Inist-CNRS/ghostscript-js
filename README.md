![logo ghostscript-js](/ghostscript-js.png)

# ghostscript-js

Just a nodeJS addons for ghostscript

## Requirements
This version has been tested only on Ubuntu 16.04

**Debian/Ubuntu**
```shell
sudo apt-get install libgs-dev g++ cmake
```

## Install
```shell
npm install ghostscript-js
```

## Usage
```javascript

const Ghostscript = require('ghostscript-js')

const gs = new Ghostscript()

gs.exec([
  '-q',
  '-dNOPAUSE',
  '-dBATCH',
  '-sDEVICE=tiff24nc',
  '-r300',
  '-sOutputFile=output-%03d.tiff',
  'input.pdf'
], (codeError) => {
  if (codeError) {
    // deal with the codeError
    ...
  } else {
    // Great ! No errors !
    ...
  }
});
```

For more information about GhostScript error messages : https://ghostscript.com/doc/current/API.htm#return_codes