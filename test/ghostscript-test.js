'use strict';
/* eslint-env mocha */

const pkg = require('./../package.json');
const childProcess = require('child_process');
const Ghostscript = require('./../src/ghostscript');
const chai = require('chai');
const expect = chai.expect;
const path = require('path');

describe(pkg.name + '/src/ghostscript.js', () => {
  describe('#constructor', () => {
    it('should initialize some attributes', () => {
      let gs = new Ghostscript();
      expect(gs.options.length).to.equal(0);
      expect(gs._input).to.deep.equal([]);
    });
  });
  describe('#nopause', () => {
    it('should set nopause option', () => {
      const gs = new Ghostscript();
      expect(gs.nopause().options[0]).to.equal('-dNOPAUSE');
    });
  });
  describe('#quiet', () => {
    it('should set quiet option', () => {
      const gs = new Ghostscript();
      expect(gs.quiet().options[0]).to.equal('-q');
    });
  });
  describe('#batch', () => {
    it('should set batch option', () => {
      const gs = new Ghostscript();
      expect(gs.batch().options[0]).to.equal('-dBATCH');
    });
  });
  describe('#interpolate', () => {
    it('should set interpolate option', () => {
      const gs = new Ghostscript();
      expect(gs.interpolate().options[0]).to.equal('-dINTERPOLATE');
    });
  });
  describe('#ram', () => {
    it('should set ram default option (tiff24nc)', () => {
      const gs = new Ghostscript();
      expect(gs.ram().lastOption[0]).to.equal('-c "30000000 setvmthreshold" -f');
    });
    it('should set ram option', (done) => {
      const gs = new Ghostscript();
      expect(gs.ram(123456789).lastOption[0]).to.equal('-c "123456789 setvmthreshold" -f');
      done();
    });
  });
  describe('#device', () => {
    it('should set device default option (tiff24nc)', () => {
      const gs = new Ghostscript();
      expect(gs.device().options[0]).to.equal('-sDEVICE=tiff24nc');
    });
    it('should set device option', (done) => {
      const gs = new Ghostscript();
      expect(gs.device('jpeg').options[0]).to.equal('-sDEVICE=jpeg');
      done();
    });
  });
  describe('#resolution', () => {
    it('should set resolution default option (300)', () => {
      const gs = new Ghostscript();
      expect(gs.resolution().options[0]).to.equal('-r300');
    });
    it('should set resolution option', () => {
      const gs = new Ghostscript();
      expect(gs.resolution(600).options[0]).to.equal('-r600');
    });
  });
  describe('#firstPage', () => {
    it('should set firstPage option', () => {
      const gs = new Ghostscript();
      expect(gs.firstPage(1).options[0]).to.equal('-dFirstPage=1');
    });
  });
  describe('#lastPage', () => {
    it('should set lastPage option', () => {
      const gs = new Ghostscript();
      expect(gs.lastPage(1).options[0]).to.equal('-dLastPage=1');
    });
  });
  describe('#input', () => {
    it('should set input option', () => {
      const gs = new Ghostscript();
      expect(gs.input('fileInput.pdf')._input).to.deep.equal(['fileInput.pdf']);
    });
  });
  describe('#output', () => {
    it('should set output option', () => {
      const gs = new Ghostscript();
      expect(gs.output('fileOutput.tiff').options[0]).to.equal('-sOutputFile=fileOutput.tiff');
    });
  });
  describe('#compatibility', () => {
    it('should set compatibility option', () => {
      const gs = new Ghostscript();
      expect(gs.compatibility(1.4).options[0]).to.equal('-dCompatibility=1.4');
    });
  });
  describe('#pdfsettings', () => {
    it('should set pdfsettings default option', () => {
      const gs = new Ghostscript();
      expect(gs.pdfsettings('/ebook').options[0]).to.equal('-dPDFSETTINGS=/ebook');
    });
  });
  describe('#autoRotatePages', () => {
    it('should set autoRotatePages default option (All)', () => {
      const gs = new Ghostscript();
      expect(gs.autoRotatePages().options[0]).to.equal('-dAutoRotatePages=/All');
    });

    it('should set autoRotatePages option', () => {
      const gs = new Ghostscript();
      expect(gs.autoRotatePages('PageByPage').options[0]).to.equal('-dAutoRotatePages=/PageByPage');
    });
  });
  describe('#antiAliasColorImage', () => {
    it('should set antiAliasColorImage option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.antiAliasColorImage(false).options[0]).to.equal('-dAntiAliasColorImage=false');
    });

    it('should set antiAliasColorImage option with string', () => {
      const gs = new Ghostscript();
      expect(gs.antiAliasColorImage('false').options[0]).to.equal('-dAntiAliasColorImage=false');
    });
  });
  describe('#antiAliasGrayImage', () => {
    it('should set antiAliasGrayImage option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.antiAliasGrayImage(false).options[0]).to.equal('-dAntiAliasGrayImage=false');
    });

    it('should set antiAliasGrayImage option with string', () => {
      const gs = new Ghostscript();
      expect(gs.antiAliasGrayImage('false').options[0]).to.equal('-dAntiAliasGrayImage=false');
    });
  });
  describe('#antiAliasMonoImage', () => {
    it('should set antiAliasMonoImage option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.antiAliasMonoImage(false).options[0]).to.equal('-dAntiAliasMonoImage=false');
    });

    it('should set antiAliasMonoImage option with string', () => {
      const gs = new Ghostscript();
      expect(gs.antiAliasMonoImage('false').options[0]).to.equal('-dAntiAliasMonoImage=false');
    });
  });
  describe('#autoFilterColorImages', () => {
    it('should set autoFilterColorImages option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.autoFilterColorImages(false).options[0]).to.equal('-dAutoFilterColorImages=false');
    });

    it('should set autoFilterColorImages option with string', () => {
      const gs = new Ghostscript();
      expect(gs.autoFilterColorImages('false').options[0]).to.equal('-dAutoFilterColorImages=false');
    });
  });
  describe('#colorImageFilter', () => {
    it('should set colorImageFilter option with string', () => {
      const gs = new Ghostscript();
      expect(gs.colorImageFilter('FlateEncode').options[0]).to.equal('-dColorImageFilter=/FlateEncode');
    });
  });
  describe('#autoFilterGrayImages', () => {
    it('should set autoFilterGrayImages option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.autoFilterGrayImages(false).options[0]).to.equal('-dAutoFilterGrayImages=false');
    });

    it('should set autoFilterGrayImages option with string', () => {
      const gs = new Ghostscript();
      expect(gs.autoFilterGrayImages('false').options[0]).to.equal('-dAutoFilterGrayImages=false');
    });
  });
  describe('#grayImageFilter', () => {
    it('should set grayImageFilter option', () => {
      const gs = new Ghostscript();
      expect(gs.grayImageFilter('FlateEncode').options[0]).to.equal('-dGrayImageFilter=/FlateEncode');
    });
  });
  describe('#downsampleColorImages', () => {
    it('should set downsampleColorImages option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.downsampleColorImages(false).options[0]).to.equal('-dDownsampleColorImages=false');
    });

    it('should set downsampleColorImages option with string', () => {
      const gs = new Ghostscript();
      expect(gs.downsampleColorImages('false').options[0]).to.equal('-dDownsampleColorImages=false');
    });
  });
  describe('#downsampleGrayImages', () => {
    it('should set downsampleGrayImages option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.downsampleGrayImages(false).options[0]).to.equal('-dDownsampleGrayImages=false');
    });

    it('should set downsampleGrayImages option with string', () => {
      const gs = new Ghostscript();
      expect(gs.downsampleGrayImages('false').options[0]).to.equal('-dDownsampleGrayImages=false');
    });
  });
  describe('#downsampleMonoImages', () => {
    it('should set downsampleMonoImages option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.downsampleMonoImages(false).options[0]).to.equal('-dDownsampleMonoImages=false');
    });

    it('should set downsampleMonoImages option with string', () => {
      const gs = new Ghostscript();
      expect(gs.downsampleMonoImages('false').options[0]).to.equal('-dDownsampleMonoImages=false');
    });
  });
  describe('#colorConversionStrategy', () => {
    it('should set colorConversionStrategy option', () => {
      const gs = new Ghostscript();
      expect(gs.colorConversionStrategy('LeaveColorUnchanged').options[0]).to.equal('-dColorConversionStrategy=/LeaveColorUnchanged');
    });
  });
  describe('#convertCMYKImagesToRGB', () => {
    it('should set convertCMYKImagesToRGB option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.convertCMYKImagesToRGB(false).options[0]).to.equal('-dConvertCMYKImagesToRGB=false');
    });

    it('should set convertCMYKImagesToRGB option with string', () => {
      const gs = new Ghostscript();
      expect(gs.convertCMYKImagesToRGB('false').options[0]).to.equal('-dConvertCMYKImagesToRGB=false');
    });
  });
  describe('#convertImagesToIndexed', () => {
    it('should set convertImagesToIndexed option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.convertImagesToIndexed(false).options[0]).to.equal('-dConvertImagesToIndexed=false');
    });

    it('should set convertImagesToIndexed option with string', () => {
      const gs = new Ghostscript();
      expect(gs.convertImagesToIndexed('false').options[0]).to.equal('-dConvertImagesToIndexed=false');
    });
  });
  describe('#UCRandBGInfo', () => {
    it('should set UCRandBGInfo option', () => {
      const gs = new Ghostscript();
      expect(gs.UCRandBGInfo('Preserve').options[0]).to.equal('-dUCRandBGInfo=/Preserve');
    });
  });
  describe('#preserveHalftoneInfo', () => {
    it('should set preserveHalftoneInfo option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.preserveHalftoneInfo(false).options[0]).to.equal('-dPreserveHalftoneInfo=false');
    });

    it('should set convertImagesToIndexed option with string', () => {
      const gs = new Ghostscript();
      expect(gs.preserveHalftoneInfo('false').options[0]).to.equal('-dPreserveHalftoneInfo=false');
    });
  });
  describe('#preserveOPIComments', () => {
    it('should set preserveOPIComments option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.preserveOPIComments(false).options[0]).to.equal('-dPreserveOPIComments=false');
    });

    it('should set preserveOPIComments option with string', () => {
      const gs = new Ghostscript();
      expect(gs.preserveOPIComments('false').options[0]).to.equal('-dPreserveOPIComments=false');
    });
  });
  describe('#preserveOverprintSettings', () => {
    it('should set preserveOverprintSettings option with boolean', () => {
      const gs = new Ghostscript();
      expect(gs.preserveOverprintSettings(false).options[0]).to.equal('-dPreserveOverprintSettings=false');
    });

    it('should set preserveOPIComments option with string', () => {
      const gs = new Ghostscript();
      expect(gs.preserveOverprintSettings('false').options[0]).to.equal('-dPreserveOverprintSettings=false');
    });
  });
  describe('#exec', () => {
    it('should convert pdf to tiff', () => {
      const gs = new Ghostscript();
      return gs.batch()
        .quiet()
        .ram()
        .nopause()
        .device()
        .resolution()
        .input(path.join(__dirname, '/data/test.pdf'))
        .output(path.join(__dirname, '/data/output/test.tif'))
        .exec();
    });

    after(() => {
      childProcess.exec(`rm ${__dirname}/data/output/test.tif`);
    });
  });
});
