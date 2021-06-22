'use strict';
/* eslint-env mocha */
/* eslint-disable no-unused-expressions */
const gs = require('./../src/ghostscript');
const rimraf = require('rimraf');
const assert = require('assert');

describe('ghostscript-js', function () {
  it('should convert some pdf to tiff', function (done) {
    gs.exec([
      '-q',
      '-dNOPAUSE',
      '-dBATCH',
      '-sDEVICE=tiff24nc',
      '-r300',
      `-sOutputFile=${__dirname}/output-%03d.tiff`,
      `${__dirname}/test.pdf`
    ], (codeError) => {
      if (codeError) return done(codeError);
      done();
    });
  });
  it('should return an code error fatal', function (done) {
    gs.exec([
      '-q',
      '-dNOPAUSE',
      '-dBATCH',
      '-sDEVICE=tiff24nc',
      '-r300',
      `-sOutputFile=${__dirname}/output-%03d.tiff`,
      `${__dirname}/no-test.pdf`
    ], (codeError) => {
      assert.strictEqual(codeError, -100);
      done();
    });
  });
  it('should throw an error "Wrong number of arguments"', function (done) {
    try {
      gs.exec([
        '-sDEVICE=tiff24nc',
        `-sOutputFile=${__dirname}/output-%03d.tiff`,
        `${__dirname}/no-test.pdf`
      ]);
    } catch (error) {
      assert.strictEqual(error.message, 'Wrong number of arguments');
      done();
    }
  });
  after(function () {
    rimraf.sync(`${__dirname}/output-*.tiff`);
  });
});
