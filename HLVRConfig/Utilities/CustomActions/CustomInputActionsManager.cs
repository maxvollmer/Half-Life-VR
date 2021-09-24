
using HLVRConfig.Utilities.Process;
using HLVRConfig.Utilities.UI;
using HLVRConfig.Utilities.UI.Config;
using HLVRConfig.Utilities.UI.Controls;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Xml;
using System.Xml.Linq;
using static HLVRConfig.Utilities.CustomActions.CustomAction;

namespace HLVRConfig.Utilities.CustomActions
{
    public class CustomInputActionsManager : IConfig
    {
        protected override I18N.I18NString DefaultLabel => new I18N.I18NString("CustomInputActions", "Custom Input Actions");

        private static JObject actionmanifest = null;
        private static JArray actions = null;
        private static List<string> customactionsfilecomments = new List<string>();

        private static Button addNewCustomActionButton = null;
        private static Button savechangesButton = null;
        private static Button discardchangesButton = null;

        private static List<CustomAction> customactions = new List<CustomAction>();

        public static void Initialize(StackPanel panel)
        {
            panel.Children.Clear();
            customactions.Clear();
            customactionsfilecomments.Clear();
            addNewCustomActionButton = null;
            savechangesButton = null;
            discardchangesButton = null;

            try
            {
                actionmanifest = JsonConvert.DeserializeObject<JObject>(File.ReadAllText(HLVRPaths.VRActionsManifest, Encoding.UTF8));
                actions = (JArray)actionmanifest.Property("actions").Value;
                string customactionsfile = File.ReadAllText(HLVRPaths.VRCustomActionsFile, Encoding.UTF8);
                using (StringReader reader = new StringReader(customactionsfile))
                {
                    Dictionary<string, List<string>> _customactions = new Dictionary<string, List<string>>();

                    string line;
                    while ((line = reader.ReadLine()) != null)
                    {
                        if (line.Trim().StartsWith("#") || line.Trim().Length == 0)
                        {
                            customactionsfilecomments.Add(line);
                            continue;
                        }

                        var pair = line.Trim().Split(new char[] { ' ' }, 2);
                        if (pair.Length == 2)
                        {
                            if (_customactions.TryGetValue(pair[0], out List<string> consolecommands))
                            {
                                consolecommands.Add(pair[1]);
                            }
                            else
                            {
                                _customactions.Add(pair[0], new List<string> { pair[1] });
                            }
                        }
                    }

                    foreach (var customaction in _customactions)
                    {
                        customactions.Add(new CustomAction(customaction.Key, customaction.Value));
                    }
                }
            }
            catch (Exception e)
            {
                panel.Children.Add(new TextBlock()
                {
                    TextWrapping = TextWrapping.Wrap,
                    Padding = new Thickness(5),
                    Margin = new Thickness(5),
                    Focusable = true,
                    MinWidth = 150,
                    Text = $"Error reading custom action files, custom actions not available. Error message:\n\n{e.Message}\n\n{e.StackTrace}"
                });
                return;
            }

            new CustomInputActionsManager().InternalInitialize(panel);
        }

        private UIElement MakeCustomActionsTitle()
        {
            StackPanel titlecontainer = new StackPanel()
            {
                Orientation = Orientation.Horizontal,
                Margin = new Thickness(5),
                HorizontalAlignment = HorizontalAlignment.Stretch
            };

            StackPanel namecontainer = new StackPanel()
            {
                Orientation = Orientation.Vertical,
                Margin = new Thickness(5),
                HorizontalAlignment = HorizontalAlignment.Stretch
            };
            titlecontainer.Children.Add(namecontainer);

            StackPanel commandscontainer = new StackPanel()
            {
                Orientation = Orientation.Vertical,
                Margin = new Thickness(5),
                HorizontalAlignment = HorizontalAlignment.Stretch
            };
            titlecontainer.Children.Add(commandscontainer);

            var namelabel = MakeTitle(new I18N.I18NString("CustomActionNameLabel", "Name"));
            namelabel.Width = 300;
            namecontainer.Children.Add(namelabel);
            var commandslabel = MakeTitle(new I18N.I18NString("CustomActionCommandsLabel", "Console Commands"));
            commandslabel.Width = 300;
            commandscontainer.Children.Add(commandslabel);

            return titlecontainer;
        }

        private void RemoveConsoleCommand(CustomAction customAction, StringHolder command, StackPanel commandcontainer)
        {
            customAction.consolecommands.Remove(command);
            if (customAction.consolecommands.Count == 0)
            {
                customAction.uielemements.ForEach(e => (e.Parent as Panel)?.Children?.Remove(e));
            }
            commandcontainer.Visibility = Visibility.Collapsed;
            (commandcontainer.Parent as Panel)?.Children?.Remove(commandcontainer);
            MarkDirty();
        }

        private void UpdateConsoleCommandText(StringHolder command, string text)
        {
            command.value = text;
            MarkDirty();
        }

        private UIElement MakeConsoleCommand(CustomAction customAction, StringHolder command)
        {
            StackPanel commandcontainer = new StackPanel()
            {
                Orientation = Orientation.Horizontal,
                Margin = new Thickness(5),
            };

            var textbox = MakeTextBox(Settings.SettingType.STRING, command.value);
            textbox.TextChanged += (object sender, TextChangedEventArgs e) =>
            {
                Application.Current?.Dispatcher?.BeginInvoke((System.Action)(() => { UpdateConsoleCommandText(command, textbox.Text); }));
            };
            commandcontainer.Children.Add(textbox);

            var removeConsoleCommandButton = new Button()
            {
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Content = new I18N.I18NString("Remove", "Remove")
            };
            removeConsoleCommandButton.Click += (object sender, RoutedEventArgs e) =>
            {
                Application.Current?.Dispatcher?.BeginInvoke((System.Action)(() => { RemoveConsoleCommand(customAction, command, commandcontainer); }));
            };
            commandcontainer.Children.Add(removeConsoleCommandButton);

            return commandcontainer;
        }

        private void UpdateCustomActionName(CustomAction customAction, string text)
        {
            customAction.name = text;
            MarkDirty();
        }

        private void AddConsoleCommand(CustomAction customAction, StackPanel commandscontainer)
        {
            StringHolder newConsoleCommandText = new StringHolder();
            customAction.consolecommands.Add(newConsoleCommandText);
            commandscontainer.Children.Insert(commandscontainer.Children.Count - 1, MakeConsoleCommand(customAction, newConsoleCommandText));
            MarkDirty();
        }

        private void MakeCustomAction(StackPanel parent, CustomAction customAction)
        {
            StackPanel actioncontainer = new StackPanel()
            {
                Orientation = Orientation.Horizontal,
                Margin = new Thickness(5),
            };

            StackPanel namecontainer = new StackPanel()
            {
                Orientation = Orientation.Vertical,
                Margin = new Thickness(5),
            };
            actioncontainer.Children.Add(namecontainer);

            StackPanel commandscontainer = new StackPanel()
            {
                Orientation = Orientation.Vertical,
                Margin = new Thickness(5),
            };
            actioncontainer.Children.Add(commandscontainer);

            var nametextbox = MakeTextBox(Settings.SettingType.STRING, customAction.name);
            nametextbox.TextChanged += (object sender, TextChangedEventArgs e) =>
            {
                Application.Current?.Dispatcher?.BeginInvoke((System.Action)(() => { UpdateCustomActionName(customAction, nametextbox.Text); }));
            };
            namecontainer.Children.Add(nametextbox);

            customAction.consolecommands.ForEach(
                command => commandscontainer.Children.Add(MakeConsoleCommand(customAction, command))
            );

            var addConsolecommandButton = new Button()
            {
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Content = new I18N.I18NString("AddConsoleCommand", "Add Console Command")
            };
            addConsolecommandButton.Click += (object sender, RoutedEventArgs e) =>
            {
                Application.Current?.Dispatcher?.BeginInvoke((System.Action)(() => { AddConsoleCommand(customAction, commandscontainer); }));
            };
            commandscontainer.Children.Add(addConsolecommandButton);

            var separator = new Separator();

            customAction.uielemements.Add(separator);
            customAction.uielemements.Add(actioncontainer);

            parent.Children.Add(separator);
            parent.Children.Add(actioncontainer);
        }

        private JObject CreateNewAction(string name)
        {
            JObject actionobject = new JObject
            {
                { "name", $"/actions/custom/in/{name}" },
                { "requirement", "optional" },
                { "type", "boolean" }
            };
            return actionobject;
        }

        private struct Action
        {
            JObject action;
            string name;

            public Action(JObject action)
            {
                this.action = action;
                this.name = (action?.Property("name")?.Value as JValue)?.Value as string;
            }

            public bool IsCustomAction()
            {
                return name?.StartsWith("/actions/custom/in/") ?? false;
            }

            public string GetName()
            {
                return name?.Substring("/actions/custom/in/".Length);
            }

            public void Remove()
            {
                action.Remove();
            }
        }

        public static bool IsCustomAction(JObject jobject)
        {
            jobject.Property("name");
            return true;
        }

        private void SyncCustomActions()
        {
            actions.Select(a => new Action(a as JObject))
                .Where(a => a.IsCustomAction())
                .ToList()
                .ForEach(a => a.Remove());

            foreach (var customaction in customactions)
            {
                actions.Add(CreateNewAction(customaction.name));
            }
        }

        private void AddNewCustomAction(StackPanel panel)
        {
            var customaction = new CustomAction();
            customactions.Add(customaction);
            MakeCustomAction(panel, customaction);
            MarkDirty();
        }

        private void InternalInitialize(StackPanel panel)
        {
            SyncCustomActions();

            panel.Children.Add(new TextBlock()
            {
                TextWrapping = TextWrapping.Wrap,
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Focusable = true,
                MinWidth = 150,
                Text =
                // TODO: Multiline i18n
                "Add custom input actions here. You can add as many as you wish.\n" +
                "Note that you need to restart the game for changes to have effect.\n\n" +
                "Each action needs a unique name, and at least one console command. " +
                "If you define multiple console commands, they are executed in a loop.\n\n" +
                "You can bind the actions in the SteamVR binding menu to your controllers or any other VR device. " +
                "When triggered, the specified console command(s) will be executed ingame."
            });

            AddButtons(panel);

            panel.Children.Add(MakeCustomActionsTitle());

            foreach (var customaction in customactions)
            {
                MakeCustomAction(panel, customaction);
            }
        }

        private void AddButtons(StackPanel panel)
        {
            StackPanel buttoncontainer = new StackPanel()
            {
                Orientation = Orientation.Horizontal,
                Margin = new Thickness(5),
            };

            addNewCustomActionButton = new Button()
            {
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Content = new I18N.I18NString("AddCustomAction", "Add Custom Action")
            };
            addNewCustomActionButton.Click += (object sender, RoutedEventArgs e) =>
            {
                Application.Current?.Dispatcher?.BeginInvoke((System.Action)(() => { AddNewCustomAction(panel); }));
            };
            buttoncontainer.Children.Add(addNewCustomActionButton);

            savechangesButton = new Button()
            {
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Content = new I18N.I18NString("SaveChanges", "Save Changes")
            };
            savechangesButton.Click += (object sender, RoutedEventArgs e) =>
            {
                Application.Current?.Dispatcher?.BeginInvoke((System.Action)(() => { SaveChanges(); }));
            };
            savechangesButton.IsEnabled = false;
            buttoncontainer.Children.Add(savechangesButton);

            discardchangesButton = new Button()
            {
                Padding = new Thickness(5),
                Margin = new Thickness(5),
                Content = new I18N.I18NString("DiscardChanges", "Discard Changes")
            };
            discardchangesButton.Click += (object sender, RoutedEventArgs e) =>
            {
                Application.Current?.Dispatcher?.BeginInvoke((System.Action)(() => { Initialize(panel); }));
            };
            discardchangesButton.IsEnabled = false;
            buttoncontainer.Children.Add(discardchangesButton);

            panel.Children.Add(buttoncontainer);
        }

        private void MarkDirty()
        {
            savechangesButton.IsEnabled = true;
            discardchangesButton.IsEnabled = true;
        }

        private bool ValidateActions(out string errors)
        {
            bool foundemptyname = false;
            errors = "";
            foreach (var customaction in customactions)
            {
                if (customaction.name.Trim().Length == 0)
                {
                    if (!foundemptyname)
                    {
                        errors += $"You have actions without a name. Actions must have a valid name.\n\n";
                        foundemptyname = true;
                    }
                }
                else if (!Regex.IsMatch(customaction.name, @"^[a-zA-Z]+$"))
                {
                    errors += $"{customaction.name} is not a valid name. Actionnames must be all letters (a-z and A-Z), no other characters are allowed.\n\n";
                }

                foreach (var consolecommand in customaction.consolecommands)
                {
                    if (consolecommand.value.Trim().Length == 0)
                    {
                        errors += $"Action {customaction.name} has empty commands. Commands cannot be empty.\n\n";
                        foundemptyname = true;
                        break;
                    }
                }
            }
            return errors.Length == 0;
        }

        private void SaveChanges()
        {
            if (!ValidateActions(out string errors))
            {
                MessageBox.Show("You have invalid data. Please correct the following issues:\n\n" + errors, "Saving Changes Failed", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            SyncCustomActions();

            File.Copy(HLVRPaths.VRActionsManifest, HLVRPaths.VRActionsManifest + ".bak");
            File.Copy(HLVRPaths.VRCustomActionsFile, HLVRPaths.VRCustomActionsFile + ".bak");

            File.WriteAllText(HLVRPaths.VRActionsManifest, JsonConvert.SerializeObject(actionmanifest, Newtonsoft.Json.Formatting.Indented), Encoding.UTF8);

            StringBuilder customactionsfile = new StringBuilder();
            customactionsfilecomments.ForEach(c => customactionsfile.AppendLine(c));
            customactions.ForEach(c => c.Write(customactionsfile));
            File.WriteAllText(HLVRPaths.VRCustomActionsFile, customactionsfile.ToString(), Encoding.UTF8);

            savechangesButton.IsEnabled = false;
            discardchangesButton.IsEnabled = false;
        }
    }
}
