using Microsoft.Collections.Extensions;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities.Settings
{
    public enum SettingType
    {
        STRING,
        BOOLEAN,
        ENUM,
        SPEED,
        DISTANCE,
        FACTOR,
        COUNT
    }

    public class Setting
    {
        public SettingType Type { get; set; } = SettingType.BOOLEAN;
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
            return Type == SettingType.BOOLEAN && !Value.Equals("0");
        }

        public void SetValue(string value)
        {
            switch (Type)
            {
                case SettingType.STRING:
                case SettingType.ENUM:
                    Value = value;
                    break;
                case SettingType.BOOLEAN:
                    Value = (value != null && value.Length > 0 && value != "0") ? "1" : "0";
                    break;
                case SettingType.SPEED:
                case SettingType.DISTANCE:
                case SettingType.FACTOR:
                    if (float.TryParse(value, out float fvalue)) Value = fvalue.ToString(); else Value = "0";
                    break;
                case SettingType.COUNT:
                    if (int.TryParse(value, out int ivalue)) Value = ivalue.ToString(); else Value = "0";
                    break;
            }
        }

        public static Setting Create(I18N.I18NString description, string value)
        {
            var setting = new Setting()
            {
                Type = SettingType.STRING,
                Value = value,
                DefaultValue = value,
                Description = description
            };
            return setting;
        }

        public static Setting Create(I18N.I18NString description, bool value)
        {
            var setting = new Setting()
            {
                Type = SettingType.BOOLEAN,
                Value = value ? "1" : "0",
                DefaultValue = value ? "1" : "0",
                Description = description
            };
            return setting;
        }

        public static Setting Create(I18N.I18NString description, OrderedDictionary<string, I18N.I18NString> allowedValues, string defaultvalue)
        {
            var setting = new Setting()
            {
                Type = SettingType.ENUM,
                Value = defaultvalue,
                DefaultValue = defaultvalue,
                Description = description
            };
            foreach (var allowedValue in allowedValues)
                setting.AllowedValues.Add(allowedValue.Key, allowedValue.Value);
            return setting;
        }

        public static Setting Create(I18N.I18NString description, SettingType type, string value)
        {
            var setting = new Setting()
            {
                Type = type,
                Value = value,
                DefaultValue = value,
                Description = description
            };
            return setting;
        }

        // default constructor for json
        public Setting()
        {
        }
    }
}
