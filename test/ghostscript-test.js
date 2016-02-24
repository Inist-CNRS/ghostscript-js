
'use strict'

const pkg = require('./../package.json'),
      child_process = require('child_process'),
      Ghostscript = require('./../src/ghostscript'),
      chai = require('chai'),
      expect = chai.expect

describe(pkg.name + '/src/ghostscript.js', () => {
  describe('#constructor', () => {
    it('should initialize some attributes', () => {
      let gs = new Ghostscript()
      expect(gs.options.length).to.equal(0)
      expect(gs._input).to.be.null
    })
  })
  describe('#nopause', () => {
    it('should set nopause option', () => {
      let gs = new Ghostscript
      expect(gs.nopause().options[0]).to.equal('-dNOPAUSE')
    })
  })
  describe('#batch', () => {
    it('should set batch option', () => {
      let gs = new Ghostscript()
      expect(gs.batch().options[0]).to.equal('-dBATCH')
    })
  })
  describe('#device', () => {
    it('should set device default option (tiff24nc)', () => {
      let gs = new Ghostscript
      expect(gs.device().options[0]).to.equal('-sDEVICE=tiff24nc')
    })
    it('should set device option', (done) => {
      let gs = new Ghostscript
      expect(gs.device('jpeg').options[0]).to.equal('-sDEVICE=jpeg')
      done()
    })
  })
  describe('#resolution', () => {
    it('should set resolution default option (300)', () => {
      let gs = new Ghostscript
      expect(gs.resolution().options[0]).to.equal('-r300')
    })
    it('should set resolution option', () => {
      let gs = new Ghostscript;
      expect(gs.resolution(600).options[0]).to.equal('-r600')
    })
  })
  describe('#firstPage', () => {
    it('should set firstPage option', () => {
      let gs = new Ghostscript()
      expect(gs.firstPage(1).options[0]).to.equal('-dFirstPage=1')
    })
  })
  describe('#lastPage', () => {
    it('should set lastPage option', () => {
      let gs = new Ghostscript()
      expect(gs.lastPage(1).options[0]).to.equal('-dLastPage=1')
    })
  })
  describe('#input', () => {
    it('should set input option', () => {
      let gs = new Ghostscript;
      expect(gs.input('fileInput.pdf')._input).to.equal('fileInput.pdf')
    })
  })
  describe('#output', () => {
    it('should set output option', () => {
      let gs = new Ghostscript
      expect(gs.output('fileOutput.tiff').options[0]).to.equal('-sOutputFile=fileOutput.tiff')
    })
  })
  describe('#exec', () => {
    it('should convert pdf to tiff', (done) => {
      let gs = new Ghostscript
      gs
        .batch()
        .nopause()
        .device()
        .resolution()
        .input(__dirname + '/data/test.pdf')
        .output(__dirname + '/data/output/test.tif')
        .exec().then((sdtout) => {
          done()
        })
    })
    after(() => {
      child_process.exec('rm ' + __dirname + '/data/output/test.tif')
    })
  })
})
