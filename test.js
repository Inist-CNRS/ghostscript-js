const gs = require('./build/Release/ghostscriptjs');
// const gs = require('./build/Debug/ghostscriptjs');

gs.exec(["-q", "-dNOPAUSE", "-dBATCH", "-sDEVICE=tiff24nc", "-r300", "-sOutputFile=test-%03d.tiff", "test.pdf"], (err, result) => {
    if (err) console.log(err);
    console.log(result)
});
