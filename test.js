const gs = require('./build/Release/ghostscript_js');
// const gs = require('./build/Debug/ghostscript_js');

gs.exec(["-q", "-dNOPAUSE", "-dBATCH", "-sDEVICE=tiff24nc", "-r300", "-sOutputFile=test-%03d.tiff", "test.pdf"], (err, result) => {
    if (err) console.log(err);
    console.log(result)
});
