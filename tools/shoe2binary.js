/*
  {

},
"meta": {
	"image": "sprites.png",
	"size": {"w": 257, "h": 279},
	"scale": "1"
}
}
*/



/*
  This node code will convert a shoebox pixi.js export (in json) to a binary format

*/

function pad(pad, str, padLeft) {
  if (typeof str === 'undefined')
    return pad;
  if (padLeft) {
    return (pad + str).slice(-pad.length);
  } else {
    return (str + pad).substring(0, pad.length);
  }
}

function u8(chr) {
    return(chr.charCodeAt(0));
}
function spaces(amount) {
    var str = '';
    for (var i = 0; i < amount; i++) {
        str+=' ';
    }
    return str;
}

var meta = {
    "image": "sprites.png",
	"size": {"w": 257, "h": 279},
	"scale": "1"
}

var frames = {
	"uno1.png": {
		"frame": {"x":109, "y":77, "w":19, "h":76},
		"spriteSourceSize": {"x":10,"y":2,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	},
	"uno10.png": {
		"frame": {"x":0, "y":0, "w":31, "h":76},
		"spriteSourceSize": {"x":7,"y":2,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	},
	"uno11.png": {
		"frame": {"x":0, "y":77, "w":30, "h":75},
		"spriteSourceSize": {"x":8,"y":3,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	},
	"uno2.png": {
		"frame": {"x":90, "y":0, "w":19, "h":76},
		"spriteSourceSize": {"x":10,"y":2,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	},
	"uno3.png": {
		"frame": {"x":88, "y":77, "w":20, "h":76},
		"spriteSourceSize": {"x":9,"y":2,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	},
	"uno4.png": {
		"frame": {"x":62, "y":77, "w":25, "h":76},
		"spriteSourceSize": {"x":10,"y":2,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	},
	"uno5.png": {
		"frame": {"x":61, "y":0, "w":28, "h":76},
		"spriteSourceSize": {"x":10,"y":2,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	},
	"uno6.png": {
		"frame": {"x":32, "y":0, "w":28, "h":76},
		"spriteSourceSize": {"x":10,"y":2,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	},
	"uno7.png": {
		"frame": {"x":32, "y":0, "w":28, "h":76},
		"spriteSourceSize": {"x":10,"y":2,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	},
	"uno8.png": {
		"frame": {"x":32, "y":0, "w":28, "h":76},
		"spriteSourceSize": {"x":10,"y":2,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	},
	"uno9.png": {
		"frame": {"x":31, "y":77, "w":30, "h":76},
		"spriteSourceSize": {"x":8,"y":2,"w":38,"h":81},
		"sourceSize": {"w":38,"h":81}
	}

}


var frames2 = {
	"Untitled.tga": {
		"frame": {"x":0, "y":0, "w":256, "h":179},
		"spriteSourceSize": {"x":0,"y":0,"w":256,"h":256},
		"sourceSize": {"w":256,"h":256}
	},
	"Untitled2.tga": {
		"frame": {"x":0, "y":182, "w":160, "h":96},
		"spriteSourceSize": {"x":0,"y":0,"w":256,"h":256},
		"sourceSize": {"w":256,"h":256}
	},
	"palette.tga": {
		"frame": {"x":0, "y":180, "w":256, "h":1},
		"spriteSourceSize": {"x":0,"y":0,"w":256,"h":1},
		"sourceSize": {"w":256,"h":1}
	},
	"palette2.tga": {
		"frame": {"x":161, "y":182, "w":16, "h":16},
		"spriteSourceSize": {"x":0,"y":0,"w":16,"h":16},
		"sourceSize": {"w":16,"h":16}
	}
}


function writeU8(data, buffer, counter) {
    buffer.writeUInt8(data, counter);
    counter+= 1;
    return counter;
}
function writeU16(data, buffer, counter) {
    buffer.writeUInt16LE(data, counter);
    counter+= 2;
    return counter;
}
function writeU32(data, buffer, counter) {
    buffer.writeUInt32LE(data, counter);
    counter+= 4;
    return counter;
}
function writePaddedString(str, length, buffer, counter) {
    if (str.length > length) {console.log("string is too long "+str)}
    var meta_title = pad(spaces(length), str, true);
    for (var i = 0; i < meta_title.length; i++) {
        counter = writeU8(u8(meta_title[i]), buffer, counter);
    }
    return counter;
}

var length = Object.keys(frames).length;


var buf = new Buffer(3+1+16+2+2+4+(length*36) );
var counter = 0;

counter = writeU8(u8('S'), buf, counter);                     // u8      SHO header
counter = writeU8(u8('H'), buf, counter);                     // u8      SHO header
counter = writeU8(u8('O'), buf, counter);                     // u8      SHO header
counter = writeU8(1, buf, counter);                           // u8      format version
counter = writePaddedString(meta.image, 16, buf, counter);    // u8 * 16 image texture name
counter = writeU16(meta.size.w, buf, counter);                // u16     image width
counter = writeU16(meta.size.h, buf, counter);                // u16     image height
counter = writeU32(length*36, buf, counter);                  // u32     block size

for (var frame in frames) {
    counter = writePaddedString(frame, 16, buf, counter);     // u8 * 16 frame name
    var data = frames[frame];
    counter = writeU16(data.frame.x, buf, counter);           // u16     frame x
    counter = writeU16(data.frame.y, buf, counter);
    counter = writeU16(data.frame.w, buf, counter);
    counter = writeU16(data.frame.h, buf, counter);

    counter = writeU16(data.spriteSourceSize.x, buf, counter);
    counter = writeU16(data.spriteSourceSize.y, buf, counter);
    counter = writeU16(data.spriteSourceSize.w, buf, counter);
    counter = writeU16(data.spriteSourceSize.h, buf, counter);

    counter = writeU16(data.sourceSize.w, buf, counter);
    counter = writeU16(data.sourceSize.h, buf, counter);

}

var fs = require('fs');
fs.writeFile("../resources/out.sho", buf, function(err) { });
