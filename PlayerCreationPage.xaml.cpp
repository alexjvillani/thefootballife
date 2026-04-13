#include "pch.h"
#include "PlayerCreationPage.xaml.h"
#include "GameState.h"
#if __has_include("PlayerCreationPage.g.cpp")
#include "PlayerCreationPage.g.cpp"
#endif

#include <random>
#include <vector>
#include <string>
#include <cwchar>
#include <cwctype>
#include <algorithm>
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
    bool PlayerCreationPage::IsValidNamePart(hstring const& text)
    {
        std::wstring value = text.c_str();
        if (value.empty())
        {
            return false;
        }

        return std::all_of(
            value.begin(),
            value.end(),
            [](wchar_t ch)
            {
                return ::iswalpha(ch) || ch == L'-';
            }
        );
    }

    bool PlayerCreationPage::TryGetPreferredNumber(int& preferredNumber)
    {
        std::wstring value = NumberTextBox().Text().c_str();
        if (value.empty())
        {
            return false;
        }

        try
        {
            int parsed = std::stoi(value);
            if (parsed < 0 || parsed > 99)
            {
                return false;
            }

            preferredNumber = parsed;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

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

    void PlayerCreationPage::UpdateGeneratedProfile(bool forceRegenerate)
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

        if (forceRegenerate || !m_hasGeneratedVariables)
        {
            m_familySituation = hstring(familyOptions[familyDist(gen)]);
            m_finances = hstring(financeOptions[financeDist(gen)]);
            m_distanceToClubKm = distanceDist(gen);
            m_heightGrowthCm = heightGrowthDist(gen);
            m_hasGeneratedVariables = true;
        }

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
        else if (forceRegenerate || m_generatedHeightCm == 0 || m_generatedWeightKg == 0)
        {
            m_generatedHeightCm = heightDist(gen);
            m_generatedWeightKg = weightDist(gen);
        }

        m_potentialHeightCm = m_generatedHeightCm + m_heightGrowthCm;
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
        UpdateGeneratedProfile(true);
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

        UpdateGeneratedProfile(false);
    }

    void PlayerCreationPage::NameField_Changed(IInspectable const& sender, RoutedEventArgs const&)
    {
        if (!m_isPageReady)
        {
            return;
        }

        if (auto textBox = sender.try_as<TextBox>())
        {
            std::wstring current = textBox.Text().c_str();
            std::wstring cleaned;
            cleaned.reserve(current.size());

            for (wchar_t ch : current)
            {
                if (::iswalpha(ch) || ch == L'-')
                {
                    cleaned.push_back(ch);
                }
            }

            if (cleaned != current)
            {
                textBox.Text(hstring(cleaned));
                textBox.SelectionStart(static_cast<int32_t>(cleaned.size()));
            }
        }

        SummaryNameText().Text(L"Name: " + GetFullName());
    }

    void PlayerCreationPage::PlayerField_Changed(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_isPageReady)
        {
            return;
        }

        std::wstring numberText = NumberTextBox().Text().c_str();
        std::wstring cleanedNumber;
        cleanedNumber.reserve(numberText.size());
        for (wchar_t ch : numberText)
        {
            if (::iswdigit(ch))
            {
                cleanedNumber.push_back(ch);
            }
        }

        if (cleanedNumber != numberText)
        {
            NumberTextBox().Text(hstring(cleanedNumber));
            NumberTextBox().SelectionStart(static_cast<int32_t>(cleanedNumber.size()));
        }

        UpdateGeneratedProfile(false);
    }

    void PlayerCreationPage::PlayerField_Changed(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (!m_isPageReady)
        {
            return;
        }

        UpdateGeneratedProfile(false);
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

        if (!IsValidNamePart(firstName) || !IsValidNamePart(lastName))
        {
            ContentDialog dialog;
            dialog.Title(box_value(L"Invalid Name"));
            dialog.Content(box_value(L"Names can only contain letters and hyphens."));
            dialog.CloseButtonText(L"OK");
            dialog.XamlRoot(this->XamlRoot());
            dialog.ShowAsync();
            return;
        }

        int preferredNumber = 0;
        if (!TryGetPreferredNumber(preferredNumber))
        {
            ContentDialog dialog;
            dialog.Title(box_value(L"Invalid Preferred Number"));
            dialog.Content(box_value(L"Preferred number must be a whole number between 0 and 99."));
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

        UpdateGeneratedProfile(false);

        GameState::CurrentPlayer.firstName = firstName.c_str();
        GameState::CurrentPlayer.lastName = lastName.c_str();
        GameState::CurrentPlayer.position = GetComboBoxValue(PositionComboBox()).c_str();
        GameState::CurrentPlayer.foot = GetComboBoxValue(FootComboBox()).c_str();
        GameState::CurrentPlayer.number = to_hstring(preferredNumber).c_str();
        GameState::CurrentPlayer.team = GetComboBoxValue(TeamComboBox()).c_str();
        GameState::CurrentPlayer.state = GetComboBoxValue(StateComboBox()).c_str();
        GameState::CurrentPlayer.schoolType = GetComboBoxValue(SchoolComboBox()).c_str();
        GameState::CurrentPlayer.region = GetComboBoxValue(RegionComboBox()).c_str();
        GameState::CurrentPlayer.familySituation = m_familySituation.c_str();
        GameState::CurrentPlayer.finances = m_finances.c_str();

        GameState::CurrentPlayer.heightCm = m_generatedHeightCm;
        GameState::CurrentPlayer.weightKg = m_generatedWeightKg;
        GameState::CurrentPlayer.potentialHeightCm = m_potentialHeightCm;
        GameState::CurrentPlayer.distanceToClubKm = m_distanceToClubKm;

        // Replace this later if you store the actual random prospect image path
        GameState::CurrentPlayer.profileImagePath = L"ms-appx:///Assets/StoreLogo.png";

        std::wstring summary =
            L"Player Created!\n\nName: " + std::wstring(GetFullName()) +
            L"\nPosition: " + std::wstring(GetComboBoxValue(PositionComboBox())) +
            L"\nFoot: " + std::wstring(GetComboBoxValue(FootComboBox())) +
            L"\nNumber: " + std::to_wstring(preferredNumber) +
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
