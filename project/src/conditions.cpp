#include "conditions.h"

#include "fields.h"
#include "text.h"

namespace nano_edr {

bool FieldEquals::operator()(const Event& event) const {
    const std::string* field = FindField(event, key_);
    return field != nullptr && TextEquals(*field, value_);
}

bool FieldContains::operator()(const Event& event) const {
    const std::string* field = FindField(event, key_);
    return field != nullptr && TextContains(*field, needle_);
}

bool FieldEndsWith::operator()(const Event& event) const {
    const std::string* field = FindField(event, key_);
    return field != nullptr && TextEndsWith(*field, suffix_);
}

bool FieldInList::operator()(const Event& event) const {
    const std::string* field = FindField(event, key_);
    if (field == nullptr) {
        return false;
    }
    for (const std::string& value : values_) {
        if (TextEquals(*field, value)) {
            return true;
        }
    }
    return false;
}

Condition Condition::Equals(const std::string& key,
                            const std::vector<std::string>& values) {
    return Condition(Match::kEquals, key, values);
}

Condition Condition::Contains(const std::string& key,
                              const std::vector<std::string>& values) {
    return Condition(Match::kContains, key, values);
}

Condition Condition::EndsWith(const std::string& key,
                              const std::vector<std::string>& values) {
    return Condition(Match::kEndsWith, key, values);
}

bool Condition::operator()(const Event& event) const {
    const std::string* field = FindField(event, key_);
    if (field == nullptr) {
        return false;
    }

    for (const std::string& value : values_) {
        switch (match_) {
            case Match::kEquals:
                if (TextEquals(*field, value)) {
                    return true;
                }
                break;
            case Match::kContains:
                if (TextContains(*field, value)) {
                    return true;
                }
                break;
            case Match::kEndsWith:
                if (TextEndsWith(*field, value)) {
                    return true;
                }
                break;
        }
    }
    return false;
}

}  // namespace nano_edr
