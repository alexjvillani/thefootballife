#include "pch.h"
#include "PlayerCreationPage.xaml.h"
#if __has_include("PlayerCreationPage.g.cpp")
#include "PlayerCreationPage.g.cpp"
#endif

#include <random>
#include <vector>
#include <string>
#include <cwchar>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;

namespace winrt::thefootballife::implementation
{
    PlayerCreationPage::PlayerCreationPage()
    {
        InitializeComponent();
        NavigationCacheMode(Microsoft::UI::Xaml::Navigation::NavigationCacheMode::Required);
        m_isPageReady = true;
        SetRandomProfileImage();
    }

    hstring PlayerCreationPage::GetComboBoxValue(ComboBox const& comboBox)
    {
        if (!comboBox)
        {
            return L"";
        }

        auto selectedItem = comboBox.SelectedItem();
        if (!selectedItem)
        {
            return L"";
        }

        if (auto item = selectedItem.try_as<ComboBoxItem>())
        {
            auto content = item.Content();
            if (content)
            {
                return unbox_value<hstring>(content);
            }
        }

        return L"";
    }

    hstring PlayerCreationPage::GetFullName()
    {
        std::wstring first = FirstNameTextBox().Text().c_str();
        std::wstring last = LastNameTextBox().Text().c_str();

        if (first.empty() && last.empty())
        {
            return L"Unnamed Prospect";
        }

        if (first.empty())
        {
            return hstring(last);
        }

        if (last.empty())
        {
            return hstring(first);
        }

        return hstring(first + L" " + last);
    }

    hstring PlayerCreationPage::FormatHeightFeet(int totalCm)
    {
        double totalInches = static_cast<double>(totalCm) / 2.54;
        int roundedInches = static_cast<int>(totalInches + 0.5);
        int feet = roundedInches / 12;
        int inches = roundedInches % 12;

        return to_hstring(feet) + L"'" + to_hstring(inches) + L"\"";
    }

    int PlayerCreationPage::ParseFeetAndInches(hstring const& text)
    {
        std::wstring value = text.c_str();
        if (value.empty())
        {
            return 0;
        }

        int feet = 0;
        int inches = 0;

        size_t apostrophePos = value.find(L'\'');
        if (apostrophePos != std::wstring::npos)
        {
            try
            {
                feet = std::stoi(value.substr(0, apostrophePos));

                size_t quotePos = value.find(L'"', apostrophePos + 1);
                std::wstring inchPart = value.substr(
                    apostrophePos + 1,
                    quotePos == std::wstring::npos ? std::wstring::npos : quotePos - apostrophePos - 1
                );

                if (!inchPart.empty())
                {
                    inches = std::stoi(inchPart);
                }
            }
            catch (...)
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }

        if (feet < 4 || feet > 8 || inches < 0 || inches > 11)
        {
            return 0;
        }

        int totalInches = (feet * 12) + inches;
        int cm = static_cast<int>((static_cast<double>(totalInches) * 2.54) + 0.5);
        return cm;
    }

    int PlayerCreationPage::ParseWeight(hstring const& text)
    {
        std::wstring value = text.c_str();
        if (value.empty())
        {
            return 0;
        }

        try
        {
            int weight = std::stoi(value);
            if (weight < 50 || weight > 150)
            {
                return 0;
            }

            return weight;
        }
        catch (...)
        {
            return 0;
        }
    }

    void PlayerCreationPage::SetRandomProfileImage()
    {
        BitmapImage bitmap;
        bitmap.UriSource(Uri(L"ms-appx:///Assets/StoreLogo.png"));
        ProfileImage().Source(bitmap);
    }

    void PlayerCreationPage::UpdateGeneratedProfile()
    {
        std::random_device rd;
        std::mt19937 gen(rd());

        std::vector<std::wstring> familyOptions =
        {
            L"Stable Home",
            L"Single Parent",
            L"Large Family",
            L"Rural Upbringing",
            L"Football Family"
        };

        std::vector<std::wstring> financeOptions =
        {
            L"Tight",
            L"Stable",
            L"Comfortable"
        };

        std::uniform_int_distribution<> familyDist(0, static_cast<int>(familyOptions.size()) - 1);
        std::uniform_int_distribution<> financeDist(0, static_cast<int>(financeOptions.size()) - 1);
        std::uniform_int_distribution<> heightDist(170, 205);
        std::uniform_int_distribution<> weightDist(68, 108);
        std::uniform_int_distribution<> heightGrowthDist(0, 8);
        std::uniform_int_distribution<> distanceDist(1, 60);

        m_familySituation = hstring(familyOptions[familyDist(gen)]);
        m_finances = hstring(financeOptions[financeDist(gen)]);
        m_distanceToClubKm = distanceDist(gen);

        auto school = GetComboBoxValue(SchoolComboBox());
        if (school == L"Private School")
        {
            m_schoolQuality = L"High";
        }
        else if (school == L"Sports Academy")
        {
            m_schoolQuality = L"Elite";
        }
        else if (school == L"Country School")
        {
            m_schoolQuality = L"Developing";
        }
        else
        {
            m_schoolQuality = L"Standard";
        }

        bool manualPhysical = false;
        auto isChecked = ManualPhysicalCheckBox().IsChecked();
        if (isChecked && isChecked.Value())
        {
            manualPhysical = true;
        }

        if (manualPhysical)
        {
            int parsedHeight = ParseFeetAndInches(HeightTextBox().Text());
            int parsedWeight = ParseWeight(WeightTextBox().Text());

            if (parsedHeight > 0)
            {
                m_generatedHeightCm = parsedHeight;
            }
            else
            {
                m_generatedHeightCm = heightDist(gen);
            }

            if (parsedWeight > 0)
            {
                m_generatedWeightKg = parsedWeight;
            }
            else
            {
                m_generatedWeightKg = weightDist(gen);
            }
        }
        else
        {
            m_generatedHeightCm = heightDist(gen);
            m_generatedWeightKg = weightDist(gen);
        }

        m_potentialHeightCm = m_generatedHeightCm + heightGrowthDist(gen);
        if (m_potentialHeightCm > 213)
        {
            m_potentialHeightCm = 213;
        }

        FamilySituationText().Text(m_familySituation);
        SetRandomProfileImage();

        auto fullName = GetFullName();

        SummaryNameText().Text(L"Name: " + fullName);
        SummaryBuildText().Text(
            L"Height / Weight: " +
            FormatHeightFeet(m_generatedHeightCm) +
            L" / " +
            to_hstring(m_generatedWeightKg) +
            L" kg"
        );
        SummaryPotentialText().Text(L"Potential Height: " + FormatHeightFeet(m_potentialHeightCm));
        SummaryDistanceText().Text(L"Distance to Club: " + to_hstring(m_distanceToClubKm) + L" km");
        SummarySchoolText().Text(L"School Quality: " + m_schoolQuality);
        SummaryFinanceText().Text(L"Finances: " + m_finances);
        SummaryFamilyText().Text(L"Family Situation: " + m_familySituation);

        std::wstring outlook = L"A ";
        outlook += GetComboBoxValue(PositionComboBox()).c_str();
        outlook += L" prospect from ";
        outlook += GetComboBoxValue(StateComboBox()).c_str();
        outlook += L" with a ";
        outlook += GetComboBoxValue(RegionComboBox()).c_str();
        outlook += L" background. ";
        outlook += L"Supporters around ";
        outlook += GetComboBoxValue(TeamComboBox()).c_str();
        outlook += L" have started to notice this young player’s upside.";

        CareerOutlookText().Text(hstring(outlook));
    }

    void PlayerCreationPage::GeneratePreview_Click(IInspectable const&, RoutedEventArgs const&)
    {
        UpdateGeneratedProfile();
    }

    void PlayerCreationPage::ManualPhysicalCheckBox_Changed(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_isPageReady)
        {
            return;
        }

        bool manual = false;
        auto isChecked = ManualPhysicalCheckBox().IsChecked();
        if (isChecked && isChecked.Value())
        {
            manual = true;
        }

        HeightTextBox().IsEnabled(manual);
        WeightTextBox().IsEnabled(manual);

        UpdateGeneratedProfile();
    }

    void PlayerCreationPage::NameField_Changed(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_isPageReady)
        {
            return;
        }

        SummaryNameText().Text(L"Name: " + GetFullName());
    }

    void PlayerCreationPage::PlayerField_Changed(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_isPageReady)
        {
            return;
        }

        UpdateGeneratedProfile();
    }

    void PlayerCreationPage::PlayerField_Changed(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (!m_isPageReady)
        {
            return;
        }

        UpdateGeneratedProfile();
    }

    void PlayerCreationPage::BackButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (Frame().CanGoBack())
        {
            Frame().GoBack();
        }
    }

    void PlayerCreationPage::ContinueButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto firstName = FirstNameTextBox().Text();
        auto lastName = LastNameTextBox().Text();

        if (firstName.empty() || lastName.empty())
        {
            ContentDialog dialog;
            dialog.Title(box_value(L"Missing Information"));
            dialog.Content(box_value(L"Please enter both a first name and last name before continuing."));
            dialog.CloseButtonText(L"OK");
            dialog.XamlRoot(this->XamlRoot());
            dialog.ShowAsync();
            return;
        }

        bool manualPhysical = false;
        auto isChecked = ManualPhysicalCheckBox().IsChecked();
        if (isChecked && isChecked.Value())
        {
            manualPhysical = true;
        }

        if (manualPhysical)
        {
            if (ParseFeetAndInches(HeightTextBox().Text()) == 0)
            {
                ContentDialog dialog;
                dialog.Title(box_value(L"Invalid Height"));
                dialog.Content(box_value(L"Please enter height in a valid format, for example 6'2\"."));
                dialog.CloseButtonText(L"OK");
                dialog.XamlRoot(this->XamlRoot());
                dialog.ShowAsync();
                return;
            }

            if (ParseWeight(WeightTextBox().Text()) == 0)
            {
                ContentDialog dialog;
                dialog.Title(box_value(L"Invalid Weight"));
                dialog.Content(box_value(L"Please enter a valid weight in kilograms."));
                dialog.CloseButtonText(L"OK");
                dialog.XamlRoot(this->XamlRoot());
                dialog.ShowAsync();
                return;
            }
        }

        UpdateGeneratedProfile();

        std::wstring summary =
            L"Player Created!\n\nName: " + std::wstring(GetFullName()) +
            L"\nPosition: " + std::wstring(GetComboBoxValue(PositionComboBox())) +
            L"\nFoot: " + std::wstring(GetComboBoxValue(FootComboBox())) +
            L"\nNumber: " + std::wstring(NumberTextBox().Text()) +
            L"\nTeam: " + std::wstring(GetComboBoxValue(TeamComboBox())) +
            L"\nState: " + std::wstring(GetComboBoxValue(StateComboBox())) +
            L"\nHeight / Weight: " + std::wstring(FormatHeightFeet(m_generatedHeightCm)) + L" / " + std::to_wstring(m_generatedWeightKg) + L" kg" +
            L"\nPotential Height: " + std::wstring(FormatHeightFeet(m_potentialHeightCm)) +
            L"\nFamily Situation: " + std::wstring(m_familySituation) +
            L"\nDistance to Club: " + std::to_wstring(m_distanceToClubKm) + L" km";

        ContentDialog dialog;
        dialog.Title(box_value(L"Continue to X-Factors"));
        dialog.Content(box_value(hstring(summary)));
        dialog.PrimaryButtonText(L"Continue");
        dialog.CloseButtonText(L"Back");
        dialog.XamlRoot(this->XamlRoot());

        auto weakThis = get_weak();

        dialog.ShowAsync().Completed(
            [weakThis](auto const& operation, auto const&)
            {
                if (auto self = weakThis.get())
                {
                    if (operation.GetResults() == ContentDialogResult::Primary)
                    {
                        self->Frame().Navigate(
                            winrt::Windows::UI::Xaml::Interop::TypeName{
                                L"thefootballife.XFactorPage",
                                winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
                            }
                        );
                    }
                }
            }
        );
    }
}
