// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020


function setInfoText(newText) {

    let elInfo = document.getElementById("info_area_text")

    if (elInfo) {
        elInfo.value = newText;
    }
}

function prependInfoText(newText) {

    let elInfo = document.getElementById("info_area_text")

    if (elInfo) {
        elInfo.value = newText + elInfo.value;
    }
}


function appendInfoText(newText) {

    let elInfo = document.getElementById("info_area_text")

    if (elInfo) {
        elInfo.value = elInfo.value + newText;
    }
}


function hasClass(ele,cls) {
  return !!ele.className.match(new RegExp('(\\s|^)'+cls+'(\\s|$)'));
}


function addClass(ele,cls) {
  if (!hasClass(ele,cls)) ele.className += " "+cls;
}

function removeClass(ele,cls) {
  if (hasClass(ele,cls)) {
    var reg = new RegExp('(\\s|^)'+cls+'(\\s|$)');
    ele.className=ele.className.replace(reg,' ');
  }
}





function render_entry_default(name_field, tool_type) {

    let iter = 0;
    let str_result = "";
    let entry = entry_get_first_of_type(tool_type);

    if (entry !== null) {
        // This mode shows multiple entries for a given tool type if present
        while (entry !== null) {
            str_result += name_field + ": " + entry.name;

            // Don't print empty versions
            if ((entry.version.length) > 0)
                str_result += ", Version: " + entry.version;

            str_result += "\n";
            entry = entry_get_next_of_type(tool_type);
            iter++;
        }
    }
    else
        if (tool_type == TYPE_TOOLS) // Only display unknown value for main tools type
            str_result += name_field + ": <unknown>\n";

    return str_result;
}



function renderOutput(filename) {

    let str_result = "";

    // OUTPUT_DEFAULT
        str_result += "File: " + filename + "\n";

        str_result += render_entry_default("Tools", TYPE_TOOLS);
        str_result += render_entry_default("Engine", TYPE_ENGINE);
        str_result += render_entry_default("Music", TYPE_MUSIC);
        str_result += render_entry_default("SoundFX", TYPE_SOUNDFX);

        str_result += "\n";

    return str_result;
}
