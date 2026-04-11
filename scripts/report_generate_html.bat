@if (@X)==(@Y) @end /* Batch/JScript hybrid
@echo off
setlocal EnableDelayedExpansion

:: Generate HTML report from JUnit XML test results
:: Usage: report_generate_html.bat <xml_path> <html_path> [configuration] [search_root]

if "%~1"=="" (
    echo Usage: report_generate_html.bat ^<xml_path^> ^<html_path^> [configuration] [search_root]
    exit /b 1
)
if "%~2"=="" (
    echo Usage: report_generate_html.bat ^<xml_path^> ^<html_path^> [configuration] [search_root]
    exit /b 1
)

set "XML_PATH=%~1"
set "HTML_PATH=%~2"
set "CONFIG=%~3"
set "SEARCH_ROOT=%~4"
if "%CONFIG%"=="" set "CONFIG=Unknown"

cscript //nologo //E:JScript "%~f0" "%XML_PATH%" "%HTML_PATH%" "%CONFIG%" "%SEARCH_ROOT%"
exit /b %errorlevel%

*** JScript starts here ***/

var args = WScript.Arguments;
    if (args.length < 3) {
        WScript.Echo("Error: Missing arguments");
        WScript.Quit(1);
    }

var xmlPath = args(0);
var htmlPath = args(1);
var config = args(2);
    var searchRoot = args.length > 3 ? args(3) : "";

var fso = new ActiveXObject("Scripting.FileSystemObject");

if (!fso.FileExists(xmlPath)) {
    WScript.Echo("Error: XML file not found: " + xmlPath);
    WScript.Quit(1);
}

try {
    var xml = new ActiveXObject("MSXML2.DOMDocument.6.0");
    xml.async = false;
    xml.load(xmlPath);

    if (xml.parseError.errorCode != 0) {
        WScript.Echo("Error parsing XML: " + xml.parseError.reason);
        WScript.Quit(1);
    }

    var testsuite = xml.selectSingleNode("//testsuite");
    if (!testsuite) {
        WScript.Echo("Error: No testsuite found in XML");
        WScript.Quit(1);
    }

    var tests = testsuite.getAttribute("tests") || "0";
    var failures = parseInt(testsuite.getAttribute("failures") || "0");
    var errors = parseInt(testsuite.getAttribute("errors") || "0");
    var time = testsuite.getAttribute("time") || "0";
    var passed = parseInt(tests) - failures - errors;

    var now = new Date();
    var dateStr = now.getFullYear() + "-" +
        ("0" + (now.getMonth()+1)).slice(-2) + "-" +
        ("0" + now.getDate()).slice(-2) + " " +
        ("0" + now.getHours()).slice(-2) + ":" +
        ("0" + now.getMinutes()).slice(-2) + ":" +
        ("0" + now.getSeconds()).slice(-2);

    var hasFailures = (failures > 0 || errors > 0);
    var summaryClass = hasFailures ? "summary has-failures" : "summary";

    var html = '<!DOCTYPE html>\n' +
'<html>\n' +
'<head>\n' +
'    <title>Test Results - ' + config + '</title>\n' +
'    <style>\n' +
'        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n' +
'        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n' +
'        h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }\n' +
'        h2 { color: #555; margin-top: 30px; }\n' +
'        table { border-collapse: collapse; width: 100%; margin-top: 10px; }\n' +
'        th, td { border: 1px solid #ddd; padding: 12px 8px; text-align: left; }\n' +
'        th { background-color: #4CAF50; color: white; }\n' +
'        tr:nth-child(even) { background-color: #f9f9f9; }\n' +
'        tr:hover { background-color: #f1f1f1; }\n' +
'        .passed { color: #28a745; font-weight: bold; }\n' +
'        .failed { color: #dc3545; font-weight: bold; }\n' +
'        .summary { margin: 20px 0; padding: 20px; background: #e8f5e9; border-radius: 5px; border-left: 4px solid #4CAF50; }\n' +
'        .summary.has-failures { background: #ffebee; border-left-color: #dc3545; }\n' +
'        .summary p { margin: 8px 0; }\n' +
'        .summary strong { color: #333; }\n' +
'        .stat { display: inline-block; margin-right: 30px; }\n' +
'    </style>\n' +
'</head>\n' +
'<body>\n' +
'    <div class="container">\n' +
'        <h1>Signal Editor - Test Results</h1>\n' +
'        <div class="' + summaryClass + '">\n' +
'            <p><strong>Configuration:</strong> ' + config + '</p>\n' +
'            <p><strong>Date:</strong> ' + dateStr + '</p>\n' +
'            <p>\n' +
'                <span class="stat"><strong>Total Tests:</strong> ' + tests + '</span>\n' +
'                <span class="stat"><strong>Passed:</strong> <span class="passed">' + passed + '</span></span>\n' +
'                <span class="stat"><strong>Failures:</strong> <span class="' + (failures > 0 ? 'failed' : 'passed') + '">' + failures + '</span></span>\n' +
'                <span class="stat"><strong>Errors:</strong> <span class="' + (errors > 0 ? 'failed' : 'passed') + '">' + errors + '</span></span>\n' +
'                <span class="stat"><strong>Time:</strong> ' + time + 's</span>\n' +
'            </p>\n' +
'        </div>\n' +
'\n' +
'        <h2>Test Cases</h2>\n' +
'        <table>\n' +
'            <tr>\n' +
'                <th>#</th>\n' +
'                <th>Executable</th>\n' +
'                <th>Suite</th>\n' +
'                <th>Test</th>\n' +
'                <th>Description</th>\n' +
'                <th>Status</th>\n' +
'                <th>Time</th>\n' +
'            </tr>\n';

    var fileCache = {};

    function escapeHtml(text) {
        return String(text || "")
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;");
    }

    function normalizeDescription(text) {
        return String(text || "")
            .replace(/\\s+/g, " ")
            .replace(/^\\s+|\\s+$/g, "");
    }

    function humanizeTestName(name) {
        var s = String(name || "");
        s = s.replace(/_/g, " ");
        s = s.replace(/([a-z])([A-Z])/g, "$1 $2");
        s = s.replace(/([A-Z]+)([A-Z][a-z])/g, "$1 $2");
        s = s.replace(/\\s+/g, " ").replace(/^\\s+|\\s+$/g, "");
        if (s.length === 0) return "Unknown";
        return s.charAt(0).toUpperCase() + s.slice(1);
    }

    function getFileLines(path) {
        if (!path) return null;
        if (fileCache.hasOwnProperty(path)) return fileCache[path];
        if (!fso.FileExists(path)) { fileCache[path] = null; return null; }
        try {
            var stream = fso.OpenTextFile(path, 1, false);
            var content = stream.ReadAll();
            stream.Close();
            fileCache[path] = content.split(/\\r?\\n/);
            return fileCache[path];
        } catch (e) { fileCache[path] = null; return null; }
    }

    function extractInlineComment(line) {
        if (!line) return "";
        var idx = line.indexOf("//");
        if (idx >= 0) return normalizeDescription(line.substring(idx + 2));
        var blockIdx = line.indexOf("/*");
        if (blockIdx >= 0) {
            var endIdx = line.indexOf("*/", blockIdx + 2);
            if (endIdx >= 0) return normalizeDescription(line.substring(blockIdx + 2, endIdx));
        }
        return "";
    }

    function extractCommentAbove(lines, lineIndex) {
        if (!lines || lineIndex < 0 || lineIndex >= lines.length) return "";
        var inline = extractInlineComment(lines[lineIndex]);
        if (inline) return inline;
        var maxLookback = 8;
        for (var i = lineIndex - 1, looked = 0; i >= 0 && looked < maxLookback; i--, looked++) {
            var line = lines[i] ? lines[i].trim() : "";
            if (line === "") continue;
            if (line.indexOf("//") >= 0) {
                var parts = [];
                for (; i >= 0; i--) {
                    var l = lines[i].trim();
                    var idx = l.indexOf("//");
                    if (idx >= 0) { parts.unshift(l.substring(idx + 2).trim()); continue; }
                    break;
                }
                return normalizeDescription(parts.join(" "));
            }
            if (line.indexOf("*/") >= 0) {
                var block = [];
                for (; i >= 0; i--) {
                    block.unshift(lines[i]);
                    if (lines[i].indexOf("/*") >= 0) break;
                }
                var text = block.join(" ").replace(/^.*\/\*/, "").replace(/\*\/.*$/, "").replace(/\*/g, " ");
                return normalizeDescription(text);
            }
            break;
        }
        return "";
    }

    function getTestDescription(filePath, lineStr, testName) {
        var desc = "";
        var lineNum = parseInt(lineStr, 10);
        if (filePath && lineNum && lineNum > 0) {
            var lines = getFileLines(filePath);
            if (lines) desc = extractCommentAbove(lines, lineNum - 1);
        }
        if (!desc) desc = humanizeTestName(testName);
        return desc;
    }

    function collectGtestFiles(folderPath, list, seen) {
        try {
            var folder = fso.GetFolder(folderPath);
            var files = new Enumerator(folder.Files);
            for (; !files.atEnd(); files.moveNext()) {
                var file = files.item();
                var name = file.Name || "";
                if (name.match(/^gtest_.*\.xml$/i)) {
                    var key = (file.Path || "").toLowerCase();
                    if (!seen[key]) { seen[key] = true; list.push(file.Path); }
                }
            }
        } catch (e) {}
    }

    function collectGtestFilesRecursive(folderPath, list, seen) {
        try {
            var folder = fso.GetFolder(folderPath);
            collectGtestFiles(folderPath, list, seen);
            var subfolders = new Enumerator(folder.SubFolders);
            for (; !subfolders.atEnd(); subfolders.moveNext()) {
                collectGtestFilesRecursive(subfolders.item().Path, list, seen);
            }
        } catch (e) {}
    }

    var gtestFiles = [];
    var gtestSeen = {};
    var parentDir = fso.GetParentFolderName(xmlPath);
    collectGtestFiles(parentDir, gtestFiles, gtestSeen);
    if (searchRoot && fso.FolderExists(searchRoot)) {
        collectGtestFilesRecursive(searchRoot, gtestFiles, gtestSeen);
    }

    var rowIndex = 1;
    if (gtestFiles.length > 0) {
        for (var gf = 0; gf < gtestFiles.length; gf++) {
            var gtestPath = gtestFiles[gf];
            var gxml = new ActiveXObject("MSXML2.DOMDocument.6.0");
            gxml.async = false;
            gxml.load(gtestPath);
            if (gxml.parseError.errorCode != 0) continue;

            var exeName = fso.GetBaseName(gtestPath).replace(/^gtest_/, "");
            var suites = gxml.selectNodes("//testsuite");
            for (var s = 0; s < suites.length; s++) {
                var suite = suites[s];
                var suiteName = suite.getAttribute("name") || "Unknown";
                var cases = suite.selectNodes("testcase");
                for (var c = 0; c < cases.length; c++) {
                    var tc = cases[c];
                    var testName = tc.getAttribute("name") || "Unknown";
                    var tcTime = tc.getAttribute("time") || "0";
                    var filePath = tc.getAttribute("file") || "";
                    var lineStr = tc.getAttribute("line") || "";
                    var description = getTestDescription(filePath, lineStr, testName);
                    var hasFail = tc.selectSingleNode("failure") || tc.selectSingleNode("error");
                    var status = hasFail ? '<span class="failed">FAILED</span>' : '<span class="passed">PASSED</span>';
                    html += '            <tr><td>' + (rowIndex++) + '</td><td>' + escapeHtml(exeName) + '</td><td>' + escapeHtml(suiteName) + '</td><td>' + escapeHtml(testName) + '</td><td>' + escapeHtml(description) + '</td><td>' + status + '</td><td>' + tcTime + 's</td></tr>\n';
                }
            }
        }
    } else {
        var testcases = xml.selectNodes("//testcase");
        for (var i = 0; i < testcases.length; i++) {
            var tc = testcases[i];
            var name = tc.getAttribute("name") || "Unknown";
            var tcTime = tc.getAttribute("time") || "0";
            var hasFail = tc.selectSingleNode("failure") || tc.selectSingleNode("error");
            var status = hasFail ? '<span class="failed">FAILED</span>' : '<span class="passed">PASSED</span>';
            html += '            <tr><td>' + (rowIndex++) + '</td><td>' + escapeHtml(name) + '</td><td></td><td></td><td>' + escapeHtml(humanizeTestName(name)) + '</td><td>' + status + '</td><td>' + tcTime + 's</td></tr>\n';
        }
    }

    html += '        </table>\n    </div>\n</body>\n</html>';

    var outFile = fso.CreateTextFile(htmlPath, true, false);
    outFile.Write(html);
    outFile.Close();

    WScript.Echo("HTML report generated: " + htmlPath);
    WScript.Quit(0);

} catch (e) {
    WScript.Echo("Error: " + e.message);
    WScript.Quit(1);
}
