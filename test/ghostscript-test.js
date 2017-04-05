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
    ], (err, result) => {
      if (err) return done(err);
      assert.equal(result, 'All is ok !');
      done();
    });
  });
  it('should return an error with no input', function (done) {
    gs.exec([
      '-q',
      '-dNOPAUSE',
      '-dBATCH',
      '-sDEVICE=tiff24nc',
      '-r300',
      `-sOutputFile=${__dirname}/output-%03d.tiff`,
      `${__dirname}/no-test.pdf`
    ], (err) => {
      assert.equal(err, 'Something is wrong, dude !');
      done();
    });
  });
  after(function () {
    rimraf.sync(`${__dirname}/output-*.tiff`);
  });
});
