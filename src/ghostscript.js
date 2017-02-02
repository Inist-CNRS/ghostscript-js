'use strict';

const childProcess = require('child_process');

class Ghostscript {
  constructor () {
    this.options = [];
    this.lastOption = [];
    this._input = [];
  }
  nopause () {
    this.options.push('-dNOPAUSE');
    return this;
  }
  quiet () {
    this.options.push('-q');
    return this;
  }
  batch () {
    this.options.push('-dBATCH');
    return this;
  }
  interpolate () {
    this.options.push('-dINTERPOLATE');
    return this;
  }
  ram (size) {
    const _size = size || 30000000;
    this.lastOption.push('-c "' + _size + ' setvmthreshold" -f');
    return this;
  }
  device (device) {
    const _device = device || 'tiff24nc';
    this.options.push('-sDEVICE=' + _device);
    return this;
  }
  resolution (resolution) {
    const _resolution = resolution || 300;
    this.options.push('-r' + _resolution);
    return this;
  }
  firstPage (number) {
    this.options.push('-dFirstPage=' + number);
    return this;
  }
  lastPage (number) {
    this.options.push('-dLastPage=' + number);
    return this;
  }
  input (pathPdf) {
    this._input.push(pathPdf);
    return this;
  }
  output (output) {
    this.options.push('-sOutputFile=' + output);
    return this;
  }
  compatibility (compatibility) {
    this.options.push('-dCompatibility=' + compatibility);
    return this;
  }
  pdfsettings (pdfsettings) {
    this.options.push('-dPDFSETTINGS=' + pdfsettings);
    return this;
  }
  autoRotatePages (setting) {
    const _setting = setting || 'All';
    this.options.push('-dAutoRotatePages=/' + _setting);
    return this;
  }
  antiAliasColorImage (setting) {
    this.options.push('-dAntiAliasColorImage=' + setting.toString());
    return this;
  }
  antiAliasGrayImage (setting) {
    this.options.push('-dAntiAliasGrayImage=' + setting.toString());
    return this;
  }
  antiAliasMonoImage (setting) {
    this.options.push('-dAntiAliasMonoImage=' + setting.toString());
    return this;
  }
  autoFilterColorImages (setting) {
    this.options.push('-dAutoFilterColorImages=' + setting.toString());
    return this;
  }
  colorImageFilter (setting) {
    this.options.push('-dColorImageFilter=/' + setting);
    return this;
  }
  autoFilterGrayImages (setting) {
    this.options.push('-dAutoFilterGrayImages=' + setting.toString());
    return this;
  }
  grayImageFilter (setting) {
    this.options.push('-dGrayImageFilter=/' + setting);
    return this;
  }
  downsampleColorImages (setting) {
    this.options.push('-dDownsampleColorImages=' + setting.toString());
    return this;
  }
  downsampleGrayImages (setting) {
    this.options.push('-dDownsampleGrayImages=' + setting.toString());
    return this;
  }
  downsampleMonoImages (setting) {
    this.options.push('-dDownsampleMonoImages=' + setting.toString());
    return this;
  }
  colorConversionStrategy (setting) {
    this.options.push('-dColorConversionStrategy=/' + setting);
    return this;
  }
  convertCMYKImagesToRGB (setting) {
    this.options.push('-dConvertCMYKImagesToRGB=' + setting.toString());
    return this;
  }
  convertImagesToIndexed (setting) {
    this.options.push('-dConvertImagesToIndexed=' + setting.toString());
    return this;
  }
  UCRandBGInfo (setting) {
    this.options.push('-dUCRandBGInfo=/' + setting);
    return this;
  }
  preserveHalftoneInfo (setting) {
    this.options.push('-dPreserveHalftoneInfo=' + setting.toString());
    return this;
  }
  preserveOPIComments (setting) {
    this.options.push('-dPreserveOPIComments=' + setting.toString());
    return this;
  }
  preserveOverprintSettings (setting) {
    this.options.push('-dPreserveOverprintSettings=' + setting.toString());
    return this;
  }
  exec () {
    if (!this._input) {
      return new Error('An input file is missing');
    }
    const args = this.options.concat(this.lastOption).concat(this._input).join(' ');
    return new Promise((resolve, reject) => {
      childProcess.exec('gs ' + args, function (error, stdout, stderr) {
        if (error) {
          reject(error);
        } else {
          resolve(stdout);
        }
      });
    });
  }
}

module.exports = Ghostscript;
