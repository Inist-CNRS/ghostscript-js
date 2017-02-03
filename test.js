const gs = require('./build/Release/ghostscript-js');

gs.exec((err, result) => {
  if (err) console.log(err)
  console.log(result)
})
