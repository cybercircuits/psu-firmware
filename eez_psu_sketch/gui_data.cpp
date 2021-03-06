/*
 * EEZ PSU Firmware
 * Copyright (C) 2015-present, Envox d.o.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "psu.h"
#include "datetime.h"
#include "profile.h"
#include "gui_internal.h"
#include "gui_edit_mode.h"
#include "gui_edit_mode_keypad.h"
#include "gui_edit_mode_step.h"
#include "channel.h"
#include "channel_coupling.h"

namespace eez {
namespace psu {
namespace gui {
namespace data {

Value g_alertMessage;
Value g_alertMessage2;

////////////////////////////////////////////////////////////////////////////////

void Value::toText(char *text, int count) const {
    text[0] = 0;

	switch (type_) {
	case VALUE_TYPE_NONE:
		break;

	case VALUE_TYPE_INT:
		util::strcatInt(text, int_);
		break;

	case VALUE_TYPE_CONST_STR:
		strncpy_P(text, const_str_, count - 1);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_STR:
		strncpy(text, str_, count - 1);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_CHANNEL_LABEL:
		snprintf_P(text, count-1, PSTR("Channel %d:"), int_);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_CHANNEL_SHORT_LABEL:
		snprintf_P(text, count-1, PSTR("Ch%d:"), int_);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_CHANNEL_BOARD_INFO_LABEL:
		snprintf_P(text, count-1, PSTR("CH%d board:"), int_);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_LESS_THEN_MIN_FLOAT:
		snprintf_P(text, count-1, PSTR("Value is less then %.2f"), float_);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_LESS_THEN_MIN_INT:
		snprintf_P(text, count-1, PSTR("Value is less then %d"), int_);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_LESS_THEN_MIN_TIME_ZONE:
		strncpy_P(text, PSTR("Value is less then -12:00"), count-1);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_GREATER_THEN_MAX_FLOAT:
		snprintf_P(text, count-1, PSTR("Value is greater then %.2f"), float_);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_GREATER_THEN_MAX_INT:
		snprintf_P(text, count-1, PSTR("Value is greater then %d"), int_);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_GREATER_THEN_MAX_TIME_ZONE:
		strncpy_P(text, PSTR("Value is greater then +14:00"), count-1);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_EVENT: 
		{
			int year, month, day, hour, minute, second;
			datetime::breakTime(event_->dateTime, year, month, day, hour, minute, second);

			int yearNow, monthNow, dayNow, hourNow, minuteNow, secondNow;
			datetime::breakTime(datetime::now(), yearNow, monthNow, dayNow, hourNow, minuteNow, secondNow);

			if (yearNow == year && monthNow == month && dayNow == day) {
				snprintf_P(text, count-1, PSTR("%c [%02d:%02d:%02d] %s"), 127 + event_queue::getEventType(event_), hour, minute, second, event_queue::getEventMessage(event_));
			} else {
				snprintf_P(text, count-1, PSTR("%c [%02d-%02d-%02d] %s"), 127 + event_queue::getEventType(event_), day, month, year % 100, event_queue::getEventMessage(event_));
			}

			text[count - 1] = 0;
		}
		break;

	case VALUE_TYPE_PAGE_INFO:
		snprintf_P(text, count-1, PSTR("Page #%d of %d"), pageInfo_.pageIndex + 1, pageInfo_.numPages);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_ON_TIME_COUNTER:
		ontime::counterToString(text, count, uint32_);
		break;

	case VALUE_TYPE_SCPI_ERROR_TEXT:
		strncpy(text, SCPI_ErrorTranslate(int16_), count - 1);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_TIME_ZONE:
		if (int16_ == 0) {
			strncpy_P(text, PSTR("GMT"), count - 1);
			text[count - 1] = 0;
		} else {
			char sign;
			int16_t value;
			if (int16_ > 0) {
				sign = '+';
				value = int16_;
			} else {
				sign = '-';
				value = -int16_;
			}
			snprintf_P(text, count-1, PSTR("%c%02d:%02d GMT"), sign, value / 100, value % 100);
			text[count - 1] = 0;
		}
		break;

	case VALUE_TYPE_YEAR:
		snprintf_P(text, count-1, PSTR("%d"), uint16_);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_MONTH:
	case VALUE_TYPE_DAY:
	case VALUE_TYPE_HOUR:
	case VALUE_TYPE_MINUTE:
	case VALUE_TYPE_SECOND:
		snprintf_P(text, count-1, PSTR("%02d"), uint8_);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_USER_PROFILE_LABEL:
		snprintf_P(text, count-1, PSTR("[ %d ]"), int_);
		text[count - 1] = 0;
		break;

	case VALUE_TYPE_USER_PROFILE_REMARK:
		profile::getName(int_, text, count);
		break;

    case VALUE_TYPE_EDIT_INFO:
        edit_mode::getInfoText(int_, text);
        break;

    case VALUE_TYPE_IP_ADDRESS:
    {
        uint8_t *bytes = (uint8_t *)&uint32_;
		snprintf_P(text, count-1, PSTR("%d.%d.%d.%d"), (int)bytes[0], (int)bytes[1], (int)bytes[2], (int)bytes[3]);
		text[count - 1] = 0;
        break;
    }

    default:
		{
			int precision = FLOAT_TO_STR_PREC;

			if (type_ == VALUE_TYPE_FLOAT_RPM) {
				precision = 0;
			}

			util::strcatFloat(text, float_, precision);

			const char *unit;

			switch (type_) {
			case VALUE_TYPE_FLOAT_VOLT:
				unit = "V";
				break;
			case VALUE_TYPE_FLOAT_AMPER:
				unit = "A";
				break;
			case VALUE_TYPE_FLOAT_WATT:
				unit = "W";
				break;
			case VALUE_TYPE_FLOAT_SECOND:
				unit = "s";
				break;
			case VALUE_TYPE_FLOAT_CELSIUS:
				unit = "oC";
				break;
			case VALUE_TYPE_FLOAT_RPM:
				unit = "rpm";
				break;
			}

			if (unit) {
				strcat(text, unit);
			}
		}
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////

int count(uint8_t id) {
    if (id == DATA_ID_CHANNELS) {
        return CH_MAX;
    } else if (id == DATA_ID_EVENT_QUEUE_EVENTS) {
        return event_queue::EVENTS_PER_PAGE;
    } else if (id == DATA_ID_PROFILES_LIST1) {
		return 4;
	} else if (id == DATA_ID_PROFILES_LIST2) {
		return 6;
	}
    return 0;
}

void select(Cursor &cursor, uint8_t id, int index) {
    if (id == DATA_ID_CHANNELS) {
        cursor.i = index;
    } else if (id == DATA_ID_EVENT_QUEUE_EVENTS) {
		cursor.i = index;
	} else if (id == DATA_ID_PROFILES_LIST1) {
		cursor.i = index;
	} else if (id == DATA_ID_PROFILES_LIST2) {
		cursor.i = 4 + index;
	} else if (id == DATA_ID_CHANNEL_COUPLING_MODE) {
        cursor.i = 0;
    }
}

Value getMin(const Cursor &cursor, uint8_t id) {
    if (id == DATA_ID_CHANNEL_U_SET || id == DATA_ID_CHANNEL_U_MON) {
        return Value(channel_coupling::getUMin(Channel::get(cursor.i)), VALUE_TYPE_FLOAT_VOLT);
    } else if (id == DATA_ID_CHANNEL_I_SET || id == DATA_ID_CHANNEL_I_MON) {
        return Value(channel_coupling::getIMin(Channel::get(cursor.i)), VALUE_TYPE_FLOAT_AMPER);
    } else if (id == DATA_ID_EDIT_VALUE) {
        return edit_mode::getMin();
    }
    return Value();
}

Value getMax(const Cursor &cursor, uint8_t id) {
    if (id == DATA_ID_CHANNEL_U_SET || id == DATA_ID_CHANNEL_U_MON) {
        return Value(channel_coupling::getUMax(Channel::get(cursor.i)), VALUE_TYPE_FLOAT_VOLT);
    } else if (id == DATA_ID_CHANNEL_I_SET || id == DATA_ID_CHANNEL_I_MON) {
        return Value(channel_coupling::getIMax(Channel::get(cursor.i)), VALUE_TYPE_FLOAT_AMPER);
    } else if (id == DATA_ID_EDIT_VALUE) {
        return edit_mode::getMax();
    }
    return Value();
}

ValueType getUnit(const Cursor &cursor, uint8_t id) {
    if (id == DATA_ID_CHANNEL_U_SET) {
        return VALUE_TYPE_FLOAT_VOLT;
    } else if (id == DATA_ID_CHANNEL_I_SET) {
        return VALUE_TYPE_FLOAT_AMPER;
    }
    return VALUE_TYPE_NONE;
}

void getList(const Cursor &cursor, uint8_t id, const Value **values, int &count) {
    if (id == DATA_ID_EDIT_STEPS) {
        return edit_mode_step::getStepValues(values, count);
    }
}

bool set(const Cursor &cursor, uint8_t id, Value value, int16_t *error) {
	if (id == DATA_ID_CHANNEL_U_SET) {
		if (value.getFloat() < channel_coupling::getUMin(Channel::get(cursor.i)) || value.getFloat() > channel_coupling::getUMax(Channel::get(cursor.i))) {
			if (error) *error = SCPI_ERROR_DATA_OUT_OF_RANGE;
			return false;
		}
		
		if (value.getFloat() > channel_coupling::getULimit(Channel::get(cursor.i))) {
			if (error) *error = SCPI_ERROR_VOLTAGE_LIMIT_EXCEEDED;
			return false;
		}
        
		if (value.getFloat() * channel_coupling::getISet(Channel::get(cursor.i)) > channel_coupling::getPowerLimit(Channel::get(cursor.i))) {
			if (error) *error = SCPI_ERROR_POWER_LIMIT_EXCEEDED;
            return false;
        }
        
		channel_coupling::setVoltage(Channel::get(cursor.i), value.getFloat());

		return true;
    } else if (id == DATA_ID_CHANNEL_I_SET) {
		if (value.getFloat() < channel_coupling::getIMin(Channel::get(cursor.i)) || value.getFloat() > channel_coupling::getIMax(Channel::get(cursor.i))) {
			if (error) *error = SCPI_ERROR_DATA_OUT_OF_RANGE;
			return false;
		}
		
		if (value.getFloat() > channel_coupling::getILimit(Channel::get(cursor.i))) {
			if (error) *error = SCPI_ERROR_CURRENT_LIMIT_EXCEEDED;
			return false;
		}
        
		if (value.getFloat() * channel_coupling::getUSet(Channel::get(cursor.i)) > channel_coupling::getPowerLimit(Channel::get(cursor.i))) {
			if (error) *error = SCPI_ERROR_POWER_LIMIT_EXCEEDED;
            return false;
        }
        
		channel_coupling::setCurrent(Channel::get(cursor.i), value.getFloat());

		return true;
    } else if (id == DATA_ID_ALERT_MESSAGE) {
        g_alertMessage = value;
        return true;
    } else if (id == DATA_ID_ALERT_MESSAGE_2) {
        g_alertMessage2 = value;
        return true;
    } else if (id == DATA_ID_EDIT_STEPS) {
        edit_mode_step::setStepIndex(value.getInt());
        return true;
    }

	if (error) *error = 0;
	return false;
}

}
}
}
} // namespace eez::psu::ui::data
