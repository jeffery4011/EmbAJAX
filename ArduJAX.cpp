/**
 * 
 * ArduJAX - Simplistic framework for creating and handling displays and controls on a WebPage served by an Arduino (or other small device).
 * 
 * Copyright (C) 2018 Thomas Friedrichsmeier
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
**/

#include "ArduJAX.h"

#define ITOA_BUFLEN 8

// statics
ArduJAXOutputDriverBase *ArduJAXBase::_driver;
char ArduJAXBase::itoa_buf[ITOA_BUFLEN];

/** @param id: The id for the element. Note that the string is not copied. Do not use a temporary string in this place. Also, do keep it short. */
ArduJAXElement::ArduJAXElement(const char* id) : ArduJAXBase() {
    _id = id;
    _flags = StatusVisible;
    revision = 0;
}

bool ArduJAXElement::sendUpdates(uint16_t since, bool first) {
    if (!changed(since)) return false;
    if (!first) _driver->printContent(",\n");
    _driver->printContent("{\n\"id\": \"");
    _driver->printContent(_id);
    _driver->printContent("\",\n\"changes\": [");
    for (int8_t i = -2; i < additionalPropertyCount(); ++i) {
        if (i != -2) _driver->printContent(",");
        _driver->printContent("[\"");
        _driver->printContent(propertyId(i));
        _driver->printContent("\", \"");
        _driver->printContent(propertyValue(i));  // TODO: This will need quote-escaping. Probably best implemented in a dedicated function of the driver.
        _driver->printContent("\"]");
    }
    _driver->printContent("]\n}");
    return true;
}

void ArduJAXElement::setVisible(bool visible) {
    if (visible == _flags & StatusVisible) return;
    setChanged();
    if (visible) _flags |= StatusVisible;
    else _flags -= _flags & StatusVisible;
}

void ArduJAXElement::setChanged() {
    revision = _driver->setChanged();
}

bool ArduJAXElement::changed(uint16_t since) {
    if ((revision + 40000) < since) revision = since + 1;    // basic overflow protection. Results in sending _all_ states at least every 40000 request cycles
    return (revision > since);
}

//////////////////////// ArduJAXContainer /////////////////////////////

ArduJAXContainer::ArduJAXContainer(ArduJAXList children) {
    _children = children;
}

void ArduJAXContainer::print() const {
    printChildren();
}

void ArduJAXContainer::printChildren() const {
    for (int i = 0; i < _children.count; ++i) {
        _children.members[i]->print();
    }
}

bool ArduJAXContainer::sendUpdates(uint16_t since, bool first) {
    for (int i = 0; i < _children.count; ++i) {
        first = first & !_children.members[i]->sendUpdates(since, first);
    }
    return first;
}

ArduJAXElement* ArduJAXContainer::findChild(const char*id) const {
    for (int i = 0; i < _children.count; ++i) {
        ArduJAXElement* child = _children.members[i]->toElement();
        if (child) {
            if (strcmp(id, child->id()) == 0) return child;
        }
        ArduJAXContainer* childlist = _children.members[i]->toContainer();
        if (childlist) {
            child = childlist->findChild(id);
            if (child) return child;
        }
    }
}

//////////////////////// ArduJAXMutableSpan /////////////////////////////

void ArduJAXMutableSpan::print() const {
    _driver->printContent("<span id=\"");
    _driver->printContent(_id);
    _driver->printContent("\">");
    if (_value) _driver->printContent(_value);  // TODO: quoting
    _driver->printContent("</span>\n");
}

const char* ArduJAXMutableSpan::value() {
    return _value;
}

const char* ArduJAXMutableSpan::valueProperty() const {
    return "innerHTML";
}

void ArduJAXMutableSpan::setValue(const char* value) {
    // TODO: Ideally we'd special case setValue() with old value == new value (noop). However, since often both old and new values are kept in the same char buffer,
    // we cannot really compare the strings - without keeping a copy, at least. Should we?
    _value = value;
    setChanged();
}


//////////////////////// ArduJAXSlider /////////////////////////////

ArduJAXSlider::ArduJAXSlider(const char* id, int16_t min, int16_t max, int16_t initial) : ArduJAXElement(id) {
    _value = initial;
    _min = min;
    _max = max;
}

void ArduJAXSlider::print() const {
    _driver->printContent("<input id=\"");
    _driver->printContent(_id);
    _driver->printContent("\" type=\"range\" min=\"");
    _driver->printContent(itoa(_min, itoa_buf, 10));
    _driver->printContent("\" max=\"");
    _driver->printContent(itoa(_max, itoa_buf, 10));
    _driver->printContent("\" value=\"");
    _driver->printContent(itoa(_value, itoa_buf, 10));
    _driver->printContent("\" onChange=\"doRequest(this.id, this.value);\"/>");
}

const char* ArduJAXSlider::value() {
    itoa(_value, itoa_buf, 10);
}

void ArduJAXSlider::updateFromDriverArg(const char* argname) {
    char buf[16];
    _driver->getArg(argname, buf, 16);
    _value = atoi(buf);
}

const char* ArduJAXSlider::valueProperty() const {
    return "value";
}

void ArduJAXSlider::setValue(int16_t value) {
    _value = value;
    setChanged();
}

//////////////////////// ArduJAXCheckButton /////////////////////////////

ArduJAXCheckButton::ArduJAXCheckButton(const char* id, const char* label, bool checked) : ArduJAXElement(id) {
    _label = label;
    _checked = checked;
    radiogroup = 0;
}

void ArduJAXCheckButton::print() const {
    _driver->printContent("<input id=\"");
    _driver->printContent(_id);
    _driver->printContent("\" type=\"");
    if (radiogroup) {
        _driver->printContent("radio");
        _driver->printContent("\" name=\"");
        _driver->printContent(radiogroup->_name);
    }
    else _driver->printContent("checkbox");
    _driver->printContent("\" value=\"t\" onChange=\"doRequest(this.id, this.checked ? 't' : 'f');\"");
    if (_checked) _driver->printContent(" checked=\"true\"");
    _driver->printContent ("/><label for=\"");
    _driver->printContent(_id);
    _driver->printContent("\">");
    _driver->printContent(_label);
    _driver->printContent("</label>");
}

const char* ArduJAXCheckButton::value() {
    return _checked ? "true" : "";
}

void ArduJAXCheckButton::updateFromDriverArg(const char* argname) {
    char buf[16];
    _driver->getArg(argname, buf, 16);
    _checked = (buf[0] == 't');
    if (_checked && radiogroup) radiogroup->selectOption(this);
}

const char* ArduJAXCheckButton::valueProperty() const {
    return "checked";
}

void ArduJAXCheckButton::setChecked(bool checked) {
    if (_checked == checked) return;
    _checked = checked;
    if (radiogroup && checked) radiogroup->selectOption(this);
    setChanged();
}

//////////////////////// ArduJAXPage /////////////////////////////

ArduJAXPage::ArduJAXPage(ArduJAXList children, const char* title, const char* header_add) : ArduJAXContainer(children) {
    _title = title;
    _header_add = 0;
}

void ArduJAXPage::print() const {
    _driver->printHeader(true);
    _driver->printContent("<HTML><HEAD><TITLE>");
    if (_title) _driver->printContent(_title);
    _driver->printContent("</TITLE>\n<SCRIPT>\n");
    _driver->printContent("var serverrevision = 0;\n"
                            "function doRequest(id='', value='') {\n"
                            "    var req = new XMLHttpRequest();\n"
                            "    req.timeout = 10000;\n"   // probably disconnected. Don't stack up request objects forever.
                            "    req.onload = function() {\n"
                            "       doUpdates(JSON.parse(req.responseText));\n"
                            "    }\n"
                            "    req.open('POST', document.URL, true);\n"
                            "    req.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');\n"
                            "    req.send('id=' + id + '&value=' + encodeURIComponent(value) + '&revision=' + serverrevision);\n"
                            "}\n");
    _driver->printContent("function doUpdates(response) {\n"
                            "    serverrevision = response.revision;\n"
                            "    var updates = response.updates;\n"
                            "    for(i = 0; i < updates.length; i++) {\n"
                            "       element = document.getElementById(updates[i].id);\n"
                            "       changes = updates[i].changes;\n"
                            "       for(j = 0; j < changes.length; ++j) {\n"
                            "          var spec = changes[j][0].split('.');\n"
                            "          var prop = element;\n"
                            "          for(k = 0; k < (spec.length-1); ++k) {\n"   // resolve nested attributes such as style.display
                            "              prop = prop[spec[k]];\n"
                            "          }\n"
                            "          prop[spec[spec.length-1]] = changes[j][1];\n"
                            "       }\n"
                            "    }\n"
                            "}\n");
    _driver->printContent("function doPoll() {\n"
                            "    doRequest();\n"  // poll == request without id
                            "}\n"
                            "setInterval(doPoll,1000);\n");
    _driver->printContent("</SCRIPT>\n");
    if (_header_add) _driver->printContent(_header_add);
    _driver->printContent("</HEAD>\n<BODY><FORM autocomplete=\"off\">\n");  // NOTE: The nasty thing about autocomplete is that it does not trigger onChange() functions,
                                                                            // but also the "restore latest settings after client reload" is questionable in our use-case.
    printChildren();
    _driver->printContent("\n</FORM></BODY></HTML>\n");
}

void ArduJAXPage::handleRequest() {
    char conversion_buf[ARDUJAX_MAX_ID_LEN];

    // handle value changes sent from client
    uint16_t client_revision = atoi(_driver->getArg("revision", conversion_buf, ARDUJAX_MAX_ID_LEN));
    const char *id = _driver->getArg("id", conversion_buf, ARDUJAX_MAX_ID_LEN);
    ArduJAXElement *element = (id[0] == '\0') ? 0 : findChild(id);
    if (element) {
        element->updateFromDriverArg("value");
    }

    // then relay value changes that have occured in the server (possibly in response to those sent)
    _driver->printHeader(false);
    _driver->printContent("{\"revision\": ");
    _driver->printContent(itoa(_driver->revision(), conversion_buf, 10));
    _driver->printContent(",\n\"updates\": [\n");
    sendUpdates(client_revision);
    _driver->printContent("\n]}\n");

    if (element) {
        element->setChanged(); // So changes sent from one client will be synced to the other clients
    }
    _driver->nextRevision();
}