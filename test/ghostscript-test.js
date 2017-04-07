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
      if (codeError) return done(err);
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
      assert.equal(codeError, -100);
      done();
    });
  });
  after(function () {
    rimraf.sync(`${__dirname}/output-*.tiff`);
  });
});
