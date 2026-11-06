/*
 * Google Apps Script backend for Smart RFID Attendance & Access System.
 *
 * Create a Google Sheet with a tab named "Sheet1".
 * Set SPREADSHEET_ID below to your spreadsheet ID.
 */

const SPREADSHEET_ID = "YOUR_GOOGLE_SPREADSHEET_ID";
const SHEET_NAME = "Sheet1";
const TIMEZONE = "Asia/Kolkata";

function doGet(e) {
  if (!e || !e.parameter || !e.parameter.uid) {
    return ContentService.createTextOutput("Received data is undefined");
  }

  const spreadsheet = SpreadsheetApp.openById(SPREADSHEET_ID);
  const sheet = spreadsheet.getSheetByName(SHEET_NAME);
  if (!sheet) {
    return ContentService.createTextOutput("Sheet tab not found");
  }

  // Add headings if the sheet is empty.
  if (sheet.getLastRow() === 0) {
    sheet.appendRow(["Name", "UID", "Status", "Mode", "Date", "Time"]);
  }

  const now = new Date();
  const date = Utilities.formatDate(now, TIMEZONE, "dd/MM/yyyy");
  const time = Utilities.formatDate(now, TIMEZONE, "HH:mm:ss");

  sheet.appendRow([
    e.parameter.name || "Unknown",
    e.parameter.uid,
    e.parameter.status || "",
    e.parameter.mode || "",
    date,
    time
  ]);

  return ContentService.createTextOutput("OK");
}
