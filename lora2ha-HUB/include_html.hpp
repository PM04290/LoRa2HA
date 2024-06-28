const char html_device[] PROGMEM = R"rawliteral(
<article>
<header>%DEVCOUNT%</header>
<div class="row">
  <label class="col-lg-1">
    @
    <input type="number" min="1" max="99" name="dev_#D#_address" value="%CNFADDRESS%">
  </label>
  <label class="col-lg-3">
    Name
    <input type="text" name="dev_#D#_name" value="%CNFNAME%" %CNFNAMEVALID% >
  </label>
  <label class="col-lg-2">
    Model
    <input type="text" name="dev_#D#_model" value="%CNFMODEL%">
  </label>
  <label class="col-lg-1">
    RL ver
    <input type="number" min="0" max="4" name="dev_#D#_rlversion" value="%CNFRLVERSION%">
  </label>
</div>
<div class="row">
  <label class="offset-1 col-1">ID</label>
  <label class="col-2">Label</label>
  <label class="col-2">Data</label>
  <label class="col-5">HA configuration</label>
</div>
%GENCHILDS%
<footer id="footer_%DEVCOUNT%">
%GENFOOTER%
</footer>
</article>
)rawliteral";

const char html_device_footer_add[] PROGMEM = R"rawliteral(
<span class="offset-10 col-2 row">
<button type="button" class="primary" id="newchild_#D#" onclick="addchild(this)">Add child</button>
</span>
)rawliteral";

const char html_device_footer_ackdevice[] PROGMEM = R"rawliteral(
<span class="offset-9 col-3 row"></span>
)rawliteral";

const char html_child[] PROGMEM = R"rawliteral(
  <label class="offset-1 col-1">
    <input type="number" name="dev_#D#_childs_#C#_id" min="0" max="60" value="%CNFC_ID%">
  </label>
  <label class="col-2">
    <input type="text" name="dev_#D#_childs_#C#_label" value="%CNFC_LABEL%" %CNFC_LABELVALID% >
  </label>
  <label class="col-2">
    <select name="dev_#D#_childs_#C#_datatype" required>
        <option value="0" %CNFC_D0%>Boolean</option>
        <option value="1" %CNFC_D1%>Int</option>
        <option value="2" %CNFC_D2%>Float</option>
        <option value="3" %CNFC_D3%>Text</option>
        <option value="4" %CNFC_D4%>Tag</option>
        <option value="5" %CNFC_D5%>Raw/Function</option>
    </select>
  </label>
  <label class="col-5">
    <input type="hidden" name="dev_#D#_childs_#C#_sensortype" value="%CNFC_STYPE_INT%">
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
    <div id="desc_dev_#D#_childs_#C#">%CNFC_STYPE_STR% %CNFC_CLASS% %CNFC_UNIT% %CNFC_EXPIRE% %CNFC_MINI% %CNFC_MAXI% %CNFC_CA% %CNFC_CB%</div>
  </label>
  <label class="col-1">
    <a href="#cancel" role="button"  class="secondary" data-target="modalha" onClick="editHA(event, #D#, #C#)">☰</a>
  </label>
)rawliteral";
