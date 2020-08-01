using Microsoft.Collections.Extensions;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using HLVRConfig.Utilities.UI;

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
    public class SettingDependency
    {
        public enum MatchMode
        {
            MUST_MATCH,
            MUST_NOT_MATCH
        }

        public readonly string Setting;
        public readonly string Value;
        public readonly MatchMode Mode;
        public SettingDependency(string setting, string value, MatchMode mode = MatchMode.MUST_MATCH)
        {
            Setting = setting;
            Value = value;
            Mode = mode;
        }

        public bool IsSatisfied()
        {
            var setting = HLVRSettingsManager.GetSetting(Setting);
            if (setting == null)
                return true;

            if ((Mode == MatchMode.MUST_MATCH) != (setting.Value == Value))
                return false;

            if (setting.Dependency != null && !setting.Dependency.IsSatisfied())
                return false;

            var category = HLVRSettingsManager.GetCategory(Setting);
            if (category!= null && category.Dependency != null && !category.Dependency.IsSatisfied())
                return false;

            return true;
        }
    }

    public class SettingCategory
    {
        public I18N.I18NString Title { get; set; }
        public I18N.I18NString Description { get; set; } = null;
        public SettingDependency Dependency { get; set; } = null;
        public SettingCategory(I18N.I18NString title)
        {
            Title = title;
        }
        public SettingCategory(I18N.I18NString title, SettingDependency dependency = null)
        {
            Title = title;
            Dependency = dependency;
        }
        public SettingCategory(I18N.I18NString title, I18N.I18NString description, SettingDependency dependency = null)
        {
            Title = title;
            Description = description;
            Dependency = dependency;
        }
    }

    public class Setting
    {
        public SettingType Type { get; set; } = SettingType.BOOLEAN;
        public I18N.I18NString Description { get; set; }
        public string DefaultValue { get; set; } = null;
        public SettingDependency Dependency { get; set; } = null;
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
                    if (I18N.TryParseFloat(value, out float fvalue)) Value = fvalue.ToString(); else Value = "0";
                    break;
                case SettingType.COUNT:
                    if (I18N.TryParseInt(value, out int ivalue)) Value = ivalue.ToString(); else Value = "0";
                    break;
            }
        }

        public static Setting Create(I18N.I18NString description, string value, SettingDependency dependency = null)
        {
            var setting = new Setting()
            {
                Type = SettingType.STRING,
                Value = value,
                DefaultValue = value,
                Description = description,
                Dependency = dependency
            };
            return setting;
        }

        public static Setting Create(I18N.I18NString description, bool value, SettingDependency dependency = null)
        {
            var setting = new Setting()
            {
                Type = SettingType.BOOLEAN,
                Value = value ? "1" : "0",
                DefaultValue = value ? "1" : "0",
                Description = description,
                Dependency = dependency
            };
            return setting;
        }

        public static Setting Create(I18N.I18NString description, OrderedDictionary<string, I18N.I18NString> allowedValues, string defaultvalue, SettingDependency dependency = null)
        {
            var setting = new Setting()
            {
                Type = SettingType.ENUM,
                Value = defaultvalue,
                DefaultValue = defaultvalue,
                Description = description,
                Dependency = dependency
            };
            foreach (var allowedValue in allowedValues)
                setting.AllowedValues.Add(allowedValue.Key, allowedValue.Value);
            return setting;
        }

        public static Setting Create(I18N.I18NString description, SettingType type, string value, SettingDependency dependency = null)
        {
            var setting = new Setting()
            {
                Type = type,
                Value = value,
                DefaultValue = value,
                Description = description,
                Dependency = dependency
            };
            return setting;
        }

        // default constructor for json
        public Setting()
        {
        }
    }
}
