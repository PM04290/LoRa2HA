const char html_device[] PROGMEM = R"rawliteral(
<article id="conf_#D#">
<header id="header_#D#">%DEVCOUNT%</header>
%GENLINE%
<div class="row">
  <label class="offset-1 col-1">ID</label>
  <label class="col-2">Label</label>
  <label class="col-2">Data</label>
  <label class="col-5">HA configuration</label>
</div>
<footer id="footer_#D#">
<!--TODO a href="#modal" role="button" class="contrast" onclick="deldev(#D#)">&#10060;</a-->
</footer>
</article>
)rawliteral";

const char html_device_line[] PROGMEM = R"rawliteral(
<div id="conf_dev_#D#" class="row">
  <label class="col-1">
    @
    <input type="number" min="1" max="99" name="dev_#D#_address" value="%CNFADDRESS%">
  </label>
  <label class="col-3">
    Name
    <input type="text" name="dev_#D#_name" value="%CNFNAME%" %CNFNAMEVALID% >
  </label>
  <label class="col-2">
    Model
    <input type="text" name="dev_#D#_model" value="%CNFMODEL%">
  </label>
%GENPAIRING%
</div>
)rawliteral";

const char html_device_pairing[] PROGMEM = R"rawliteral(
  <label class="offset-2 col-2">
    New address
    <div style="display: inline-flex;"> 
    <input type="number" min="1" max="99" id="newadr_#D#" value="%CNFADDRESS%" />
    <input type="button" value="Change" onclick="changeAdr(#D#)" />
    </div>
  </label>
  <label class="col-2">
    &nbsp;<input type="button" value="Finish pairing" onclick="endPairing(#D#)" />
  </label>
)rawliteral";

const char html_child_line[] PROGMEM = R"rawliteral(
  <label class="offset-1 col-1">
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
  <label class="col-5">
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
