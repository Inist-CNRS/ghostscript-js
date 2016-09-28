'use strict';

const child_process = require('child_process');

class Ghostscript {
  constructor() {
    this.options = [];
    this._input = null
  }
  nopause() {
    this.options.push('-dNOPAUSE');
    return this
  }
  quiet() {
    this.options.push('-q');
    return this
  }
  batch() {
    this.options.push('-dBATCH');
    return this
  }
  ram(size) {
    const _size = size || 30000000;
    this.options.push('-c "' + _size + ' setvmthreshold"');
    this.options.push('-f');
    return this
  }
  device(device) {
    const _device = device || 'tiff24nc';
    this.options.push('-sDEVICE=' + _device);
    return this;
  }
  resolution(resolution) {
    const _resolution = resolution || 300;
    this.options.push('-r' + _resolution);
    return this
  }
  firstPage(number) {
    this.options.push('-dFirstPage=' + number);
    return this
  }
  lastPage(number) {
    this.options.push('-dLastPage=' + number);
    return this
  }
  input(pathPdf) {
    this._input = pathPdf;
    return this
  }
  output(output) {
    this.options.push('-sOutputFile=' + output);
    return this
  }
  exec() {
    if (!this._input) {
      return new Error("Il manque un fichier en entrée")
    }
    const args = this.options.concat(this._input).join(' ');
    return new Promise((resolve, reject) => {
      child_process.exec('gs ' + args, function(error, stdout, stderr) {
        if (error) {
          reject(stderr)
        } else {
          resolve(stdout)
        }
      })
    })
  }
}

module.exports = Ghostscript;
