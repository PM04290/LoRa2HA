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
    <input type="text" name="dev_#D#_name" value="%CNFNAME%">
  </label>
</div>
%GENCHILDS%
<footer>
  <span class="offset-10 col-2 row">
  <button type="button" class="primary" id="newchild_#D#" onclick="addchild(this)">Add child</button>
  </span>
</footer>
</article>
)rawliteral";

const char html_child[] PROGMEM = R"rawliteral(
  <label class="offset-1 col-1">
    %TITLEC_ID%
    <input type="number" name="dev_#D#_childs_#C#_id" min="0" max="60" value="%CNFC_ID%">
  </label>
  <label class="col-2">
    %TITLEC_NAME%
    <input type="text" name="dev_#D#_childs_#C#_label" value="%CNFC_LABEL%">
  </label>
  <label class="col-2">
    %TITLEC_TYPE%
    <select name="dev_#D#_childs_#C#_sensortype" required>
        <option value="0" %CNFC_S0%>Binary sensor</option>
        <option value="1" %CNFC_S1%>Numeric sensor</option>
        <option value="2" %CNFC_S2%>Switch</option>
        <option value="3" %CNFC_S3%>Light</option>
        <option value="4" %CNFC_S4%>Cover</option>
        <option value="5" %CNFC_S5% disabled>Fan</option>
        <option value="6" %CNFC_S6% disabled>HVac</option>
        <option value="7" %CNFC_S7% disabled>Select</option>
        <option value="8" %CNFC_S8%>Trigger</option>
        <option value="9" %CNFC_S9% disabled>Custom</option>
        <option value="10" %CNFC_S10%>Tag</option>
        <option value="11" %CNFC_S11%>Text</option>
    </select>
  </label>
  <label class="col-2">
    %TITLEC_DATA%
    <select name="dev_#D#_childs_#C#_datatype" required>
        <option value="0" %CNFC_D0%>Boolean</option>
        <option value="1" %CNFC_D1%>Int</option>
        <option value="2" %CNFC_D2%>Float</option>
        <option value="3" %CNFC_D3%>Text</option>
        <option value="4" %CNFC_D4%>Tag</option>
        <option value="5" %CNFC_D5%>Raw/Function</option>
    </select>
  </label>
  <label class="col-3">
    %TITLEC_CLASS%
    <input type="text" name="dev_#D#_childs_#C#_class" list="classlist" value="%CNFC_CLASS%">
  </label>
  <label class="col-1">
    %TITLEC_UNIT%
    <input type="text" class="form-select" name="dev_#D#_childs_#C#_unit" list="unitlist" value="%CNFC_UNIT%">
  </label>
)rawliteral";
