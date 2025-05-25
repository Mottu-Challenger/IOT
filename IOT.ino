function doPost(e) {
  try {
    var dados = JSON.parse(e.postData.contents);

    var tagID = dados.tagID || "N/A";
    var action = dados.action || "N/A";
    var status = dados.status || "N/A";

    var planilha = SpreadsheetApp.getActiveSpreadsheet();
    var aba = planilha.getSheetByName("Registros");

    if (!aba) {
      aba = planilha.insertSheet("Registros");
      aba.appendRow(["Data e Hora", "Tag ID", "Ação", "Status"]);
    }

    var dataHora = new Date();

    aba.appendRow([dataHora, tagID, action, status]);

    return ContentService
      .createTextOutput(JSON.stringify({result:"sucesso"}))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (erro) {
    return ContentService
      .createTextOutput(JSON.stringify({result:"erro", message: erro.message}))
      .setMimeType(ContentService.MimeType.JSON);
  }
}
