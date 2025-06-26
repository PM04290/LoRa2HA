const char html_device_form[] PROGMEM = R"rawliteral(
<form action="/doconfig" id="form_#D#" role="form" onsubmit="postdev(event)">
<article id="conf_#D#">
<header id="header_#D#">
%GENHEADER%
</header>
<div id="pairing_#D#" class="row">
%GENPAIRING%
</div>
<div class="row">
  <label class="col-1">ID</label>
  <label class="col-2">Label</label>
  <label class="col-2">Data</label>
  <label class="col-6">HA configuration</label>
</div>
<footer id="footer_#D#">
<div class="row">
<div class="col-1"><input class="secondary" type="submit" value="Save"></div>
<div class="offset-10 col-1"><a href="#modal" role="button" data-target="modalha" onclick="deldev(event, #D#)" style="float: right">🗑</a></div>
</div>
</footer>
</article>
</form>
)rawliteral";

const char html_device_header[] PROGMEM = R"rawliteral(
<div class="row">
<div class="col-6">
➜ <kbd>#D#</kbd> ┉ <kbd>%CNFMODEL%</kbd> ┉ <kbd>%CNFNAME%</kbd>
<input type="hidden" id="cnfdev" name="cnfdev" value="#D#">
<input type="hidden" name="dev_#D#_address" value="CNFADDRESS">
<input type="hidden" name="dev_#D#_name" value="%CNFMODEL%">
<input type="hidden" name="dev_#D#_model" value="%CNFNAME%">
</div>
</div>
)rawliteral";

const char html_device_pairing[] PROGMEM = R"rawliteral(
<div class="row" style="padding-top: 0.5rem;border: red 2px solid;border-radius: var(--border-radius);">
<div class="col-1"><input type="number" min="10" max="239" id="newadr_#D#" value="%CNFNEWADR%"></div>
<div class="col-2"><input type="button" value="Change Nr" onclick="changeAdr(#D#)"></div>
<div class="offset-2 col-2"><input type="button" value="Finish pairing" onclick="endPairing(#D#)"></div>
</div>
)rawliteral";

const char html_child_line[] PROGMEM = R"rawliteral(
  <label class="col-1">
    <input type="text" name="dev_#D#_childs_#C#_id" value="%CNFC_ID%" readonly>
    <input type="hidden" name="dev_#D#_childs_#C#_datatype" value="%CNFC_DTYPE%">
  </label>
  <label class="col-2">
    <input type="text" name="dev_#D#_childs_#C#_label" value="%CNFC_LABEL%" %CNFC_LABELVALID% >
  </label>
  <label class="col-2">
    <input type="hidden" name="dev_#D#_childs_#C#_sensortype" value="%CNFC_STYPE_INT%">
    <input type="text" value="%CNFC_STYPE_STR%" readonly >
  </label>
  <label class="col-6">
    <input type="hidden" name="dev_#D#_childs_#C#_class" value="%CNFC_CLASS%">
    <input type="hidden" name="dev_#D#_childs_#C#_category" value="%CNFC_CATEGORY%">
    <input type="hidden" name="dev_#D#_childs_#C#_unit" value="%CNFC_UNIT%">
    <input type="hidden" name="dev_#D#_childs_#C#_expire" value="%CNFC_EXPIRE%">
    <input type="hidden" name="dev_#D#_childs_#C#_options" value="%CNFC_OPT%">
    <input type="hidden" name="dev_#D#_childs_#C#_imin" value="%CNFC_IMIN%">
    <input type="hidden" name="dev_#D#_childs_#C#_imax" value="%CNFC_IMAX%">
    <input type="hidden" name="dev_#D#_childs_#C#_idiv" value="%CNFC_IDIV%">
    <input type="hidden" name="dev_#D#_childs_#C#_min" value="%CNFC_MINI%">
    <input type="hidden" name="dev_#D#_childs_#C#_max" value="%CNFC_MAXI%">
    <input type="hidden" name="dev_#D#_childs_#C#_coefa" value="%CNFC_CA%">
    <input type="hidden" name="dev_#D#_childs_#C#_coefb" value="%CNFC_CB%">
    <div id="desc_dev_#D#_childs_#C#" style="padding:0.15rem 0">%CNFC_DETAIL%</div>
  </label>
  <label class="col-1" style="text-align: end;">
    <a href="#modal" role="button" class="secondary" data-target="modalha" onClick="editHA(event, #D#, #C#)" %CNFC_BTN%>☰</a>
  </label>
)rawliteral";
