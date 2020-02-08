using Microsoft.Collections.Extensions;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities.Settings
{
    public class Setting
    {
        public enum SettingValueType
        {
            STRING,
            BOOLEAN,
            FLOAT,
            INT
        }

        public SettingValueType Type { get; set; } = SettingValueType.BOOLEAN;
        public I18N.I18NString Description { get; set; }
        public string DefaultValue { get; set; } = null;
        public string Value { get; set; } = null;
        public readonly OrderedDictionary<string, I18N.I18NString> AllowedValues = new OrderedDictionary<string, I18N.I18NString>();

        public bool IsFolder { get; set; } = false;

        public Setting MakeFolderSetting()
        {
            IsFolder = true;
            return this;
        }

        public bool IsTrue()
        {
            return Type == SettingValueType.BOOLEAN && !Value.Equals("0");
        }

        public void SetValue(string value)
        {
            switch (Type)
            {
                case SettingValueType.STRING:
                    Value = value;
                    break;
                case SettingValueType.BOOLEAN:
                    Value = (value != null && value.Length > 0 && value != "0") ? "1" : "0";
                    break;
                case SettingValueType.FLOAT:
                    if (float.TryParse(value, out float fvalue)) Value = fvalue.ToString(); else Value = "0";
                    break;
                case SettingValueType.INT:
                    if (int.TryParse(value, out int ivalue)) Value = ivalue.ToString(); else Value = "0";
                    break;
            }
        }

        public static Setting Create(I18N.I18NString description, bool value, OrderedDictionary<string, I18N.I18NString> allowedValues = null)
        {
            var setting = new Setting()
            {
                Type = SettingValueType.BOOLEAN,
                Value = value ? "1" : "0",
                DefaultValue = value ? "1" : "0",
                Description = description
            };
            if (allowedValues != null)
            {
                foreach (var allowedValue in allowedValues)
                    setting.AllowedValues.Add(allowedValue.Key, allowedValue.Value);
            }
            return setting;
        }

        public static Setting Create(I18N.I18NString description, int value, OrderedDictionary<string, I18N.I18NString> allowedValues = null)
        {
            var setting = new Setting()
            {
                Type = SettingValueType.INT,
                Value = value.ToString(),
                DefaultValue = value.ToString(),
                Description = description
            };
            if (allowedValues != null)
            {
                foreach (var allowedValue in allowedValues)
                    setting.AllowedValues.Add(allowedValue.Key, allowedValue.Value);
            }
            return setting;
        }

        public static Setting Create(I18N.I18NString description, float value, OrderedDictionary<string, I18N.I18NString> allowedValues = null)
        {
            var setting = new Setting()
            {
                Type = SettingValueType.FLOAT,
                Value = value.ToString(),
                DefaultValue = value.ToString(),
                Description = description
            };
            if (allowedValues != null)
            {
                foreach (var allowedValue in allowedValues)
                    setting.AllowedValues.Add(allowedValue.Key, allowedValue.Value);
            }
            return setting;
        }

        public static Setting Create(I18N.I18NString description, string value, OrderedDictionary<string, I18N.I18NString> allowedValues = null)
        {
            var setting = new Setting()
            {
                Type = SettingValueType.STRING,
                Value = value,
                DefaultValue = value,
                Description = description
            };
            if (allowedValues != null)
            {
                foreach (var allowedValue in allowedValues)
                    setting.AllowedValues.Add(allowedValue.Key, allowedValue.Value);
            }
            return setting;
        }

        // default constructor for json
        public Setting()
        {
        }
    }
}
