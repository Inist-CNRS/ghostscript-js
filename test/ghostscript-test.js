const gs = require('./../src/ghostscript');
const rimraf = require('rimraf');

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
      done();
    });
  });
  after(function () {
    rimraf.sync(`${__dirname}/output-*.tiff`);
  });
});
