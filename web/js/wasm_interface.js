// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2025

var Module;
var romusageInputFilename;

Module = {
preRun: [],
postRun: [],
print: (function() {
  return function(text) {
    if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
    // Display text output from program in text area
    var element = document.getElementById('info_area_text');
    if (element) {
      element.value += text + "\n";
      // element.scrollTop = element.scrollHeight; // focus on bottom
    }
  };
})()
};


function invokeProgram(e) {
    let fileBuffer = e.target.result;
    let uint8FileBytes = new Uint8Array(fileBuffer)

    // Clear previous output
    setInfoText("\n");
    // prependInfoText("---------------------------------\n");

    // Set up arguments from input field
    let args = document.getElementById("romusage_args").value.split(" ");
    args.push(romusageInputFilename);

    // Import function from Emscripten generated file and call it
    romusage_set_option_is_web_mode = Module.cwrap(
      'set_option_is_web_mode', null, []
    );
    romusage_set_option_is_web_mode();

    // Create a file in the virtual file system for passing in the data
    let fileNode = Module['FS_createDataFile'](".", romusageInputFilename, uint8FileBytes, true, true);

    // console.log(args);
    // Call main() in the program
    callMain(args);

    // Delete the file now that processing is done
    Module['FS_unlink'](romusageInputFilename);
}


function romusageShowHelp() {
    // Clear previous output
    setInfoText("\n");

    // Call romusage with help argument
    let args = ['-h'];
    callMain(args);
}


/*
// Old code for reference, could be deleted (see discarded: file_io.c/h)
//
// Shared buffer method of passing data, instead of using the "Advanced File System API"
// Which seems like it's semi-unsupported and not well documented

function invokeProgram(e) {
    let fileBuffer = e.target.result;
    let uint8FileBytes = new Uint8Array(fileBuffer)

    // Allocate memory in wasm matching file size and copy the file bytes into it.
    // This gives a pointer which can be passed in for access
    const sharedBufferPtr = Module._malloc(uint8FileBytes.length);
    const wasmHeapMemoryView = new Uint8Array(Module.HEAPU8.buffer, sharedBufferPtr, uint8FileBytes.length);
    wasmHeapMemoryView.set(uint8FileBytes);

    // Import function from Emscripten generated file (null is for void return type)
    // then pass in the buffer pointer and size
    romusage_web_buffer_set = Module.cwrap(
      'web_buffer_set', null, ['number', 'number']
    );
    romusage_web_buffer_set(sharedBufferPtr, uint8FileBytes.length);

    // Set up arguments from input field
    let args = document.getElementById("romusage_args").value.split(" ");
    args.push(romusageInputFilename);

    // console.log(args);
    callMain(args);

    // TODO: otherwise memory grows huge
    // Free allocated buffer
    // if (sharedBufferPtr) = Module._free(sharedBufferPtr);
}
*/