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
  <label class="col-lg-1">
    RL ver
    <input type="number" min="0" max="4" name="dev_#D#_rlversion" value="%CNFRLVERSION%">
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
  <label class="col-5">
    %TITLEC_HA%
    <input type="hidden" name="dev_#D#_childs_#C#_sensortype" value="%CNFC_STYPE_INT%">
    <input type="hidden" name="dev_#D#_childs_#C#_class" value="%CNFC_CLASS%">
    <input type="hidden" name="dev_#D#_childs_#C#_unit" value="%CNFC_UNIT%">
    <input type="hidden" name="dev_#D#_childs_#C#_expire" value="%CNFC_EXPIRE%">
    <input type="hidden" name="dev_#D#_childs_#C#_min" value="%CNFC_MINI%">
    <input type="hidden" name="dev_#D#_childs_#C#_max" value="%CNFC_MAXI%">
    <nav>
     <ul>
      <li id="li_dev_#D#_childs_#C#_sensortype">%CNFC_STYPE_STR%</li>
      <li id="li_dev_#D#_childs_#C#_class">%CNFC_CLASS%</li>
      <li id="li_dev_#D#_childs_#C#_unit">%CNFC_UNIT%</li>
      <li id="li_dev_#D#_childs_#C#_expire">%CNFC_EXPIRE%</li>
      <li id="li_dev_#D#_childs_#C#_min">%CNFC_MINI%</li>
      <li id="li_dev_#D#_childs_#C#_max">%CNFC_MAXI%</li>
     </ul>
    </nav>
  </label>
  <label class="col-1">
    <a href="#cancel" role="button"  class="secondary" data-target="modalha" onClick="editHA(event, #D#, #C#)">☰</a>
  </label>
)rawliteral";
