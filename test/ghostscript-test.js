'use strict';

const pkg = require('./../package.json'),
      child_process = require('child_process'),
      Ghostscript = require('./../src/ghostscript'),
      chai = require('chai'),
      expect = chai.expect;

describe(pkg.name + '/src/ghostscript.js', () => {
  describe('#constructor', () => {
    it('should initialize some attributes', () => {
      let gs = new Ghostscript();
      expect(gs.options.length).to.equal(0);
      expect(gs._input).to.be.null
    })
  });
  describe('#nopause', () => {
    it('should set nopause option', () => {
      const gs = new Ghostscript;
      expect(gs.nopause().options[0]).to.equal('-dNOPAUSE')
    })
  });
  describe('#quiet', () => {
    it('should set quiet option', () => {
      const gs = new Ghostscript;
      expect(gs.quiet().options[0]).to.equal('-q')
    })
  });
  describe('#batch', () => {
    it('should set batch option', () => {
      const gs = new Ghostscript();
      expect(gs.batch().options[0]).to.equal('-dBATCH')
    })
  });
  describe('#interpolate', () => {
    it('should set interpolate option', () => {
      const gs = new Ghostscript();
      expect(gs.interpolate().options[0]).to.equal('-dINTERPOLATE')
    })
  });
  describe('#ram', () => {
    it('should set ram default option (tiff24nc)', () => {
      const gs = new Ghostscript;
      expect(gs.ram().lastOption[0]).to.equal('-c "30000000 setvmthreshold" -f');
    });
    it('should set ram option', (done) => {
      const gs = new Ghostscript;
      expect(gs.ram(123456789).lastOption[0]).to.equal('-c "123456789 setvmthreshold" -f');
      done()
    })
  });
  describe('#device', () => {
    it('should set device default option (tiff24nc)', () => {
      const gs = new Ghostscript;
      expect(gs.device().options[0]).to.equal('-sDEVICE=tiff24nc')
    });
    it('should set device option', (done) => {
      const gs = new Ghostscript;
      expect(gs.device('jpeg').options[0]).to.equal('-sDEVICE=jpeg');
      done()
    })
  });
  describe('#resolution', () => {
    it('should set resolution default option (300)', () => {
      const gs = new Ghostscript;
      expect(gs.resolution().options[0]).to.equal('-r300')
    });
    it('should set resolution option', () => {
      const gs = new Ghostscript;
      expect(gs.resolution(600).options[0]).to.equal('-r600')
    })
  });
  describe('#firstPage', () => {
    it('should set firstPage option', () => {
      const gs = new Ghostscript();
      expect(gs.firstPage(1).options[0]).to.equal('-dFirstPage=1')
    })
  });
  describe('#lastPage', () => {
    it('should set lastPage option', () => {
      const gs = new Ghostscript();
      expect(gs.lastPage(1).options[0]).to.equal('-dLastPage=1')
    })
  });
  describe('#input', () => {
    it('should set input option', () => {
      const gs = new Ghostscript;
      expect(gs.input('fileInput.pdf')._input).to.equal('fileInput.pdf')
    })
  });
  describe('#output', () => {
    it('should set output option', () => {
      const gs = new Ghostscript;
      expect(gs.output('fileOutput.tiff').options[0]).to.equal('-sOutputFile=fileOutput.tiff')
    })
  });
  describe('#compatibility', () => {
    it('should set compatibility option', () => {
      const gs = new Ghostscript;
      expect(gs.compatibility(1.4).options[0]).to.equal('-dCompatibility=1.4')
    })
  });
  describe('#pdfsettings', () => {
    it('should set pdfsettings option', () => {
      const gs = new Ghostscript;
      expect(gs.pdfsettings('/ebook').options[0]).to.equal('-dPDFSETTINGS=/ebook')
    })
  });
  describe('#exec', () => {
    it('should convert pdf to tiff', () => {
      const gs = new Ghostscript;
      return gs.batch()
        .quiet()
        .ram()
        .nopause()
        .device()
        .resolution()
        .input(__dirname + '/data/test.pdf')
        .output(__dirname + '/data/output/test.tif')
        .exec()
    });
    after(() => {
      child_process.exec('rm ' + __dirname + '/data/output/test.tif')
    })
  })
});
