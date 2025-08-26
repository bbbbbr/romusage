// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2025


// Load a file into a uint8 binary array
// and  pass it in for processing
function loadFile(fileToRead) {

    // console.log(fileToRead);

    if (window.File && window.FileReader && window.FileList && window.Blob) {
        let fileReader = new FileReader();
        fileReader.argFilename = fileToRead.name; // Attach filename as custom property to event caller
        fileReader.addEventListener("load", invokeProgram);
        fileReader.readAsArrayBuffer(fileToRead);
    }
}


function buttonLoadFile(e) {

    // Load the files
    for (var i = 0; i < this.files.length; i++) {
        loadFile( this.files[i] );
    }

    // Clear value so next onchange event registers even if it is the same file
    this.value = null;
}

function dropFileHandler(ev) {

    // Clear output window
    setInfoText("\n");

    // Prevent default behavior (Prevent file from being opened)
    ev.preventDefault();

    // Handle single or multiple type drag-and-drop events
    if (ev.dataTransfer.items) {

        // Use DataTransferItemList interface to access the file(s)
        for (var i = 0; i < ev.dataTransfer.items.length; i++) {

            // Only accept file type items
            if (ev.dataTransfer.items[i].kind === 'file') {
                loadFile( ev.dataTransfer.items[i].getAsFile() );
            }
        }
    } else {
        // Use DataTransfer interface to access the file(s)
        for (var i = 0; i < ev.dataTransfer.files.length; i++) {
            loadFile( ev.dataTransfer.files[i] );
        }
    }

    // Clear the drag-and-drop style highlight
    removeClass(this, "dragdrop_ready");
}


function dragOverHandler(ev) {
    // console.log('File(s) in drop zone');

    // Highlight the drag-and-drop area with a style
    addClass(this, "dragdrop_ready");

    // Prevent default behavior (file being opened)
    ev.preventDefault();
}


function dragLeaveHandler(ev) {
    // console.log("dragLeaveHandler");

    // Clear the drag-and-drop style highlight
    removeClass(this, "dragdrop_ready");
}
