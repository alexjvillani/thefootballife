#include "pch.h"
#include "XFactorPage.xaml.h"
#include "PlayerProfileState.h"
#if __has_include("XFactorPage.g.cpp")
#include "XFactorPage.g.cpp"
#endif

#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::thefootballife::implementation
{
    XFactorPage::XFactorPage()
    {
        InitializeComponent();
        m_isPageReady = true;

        if (MentalityDescriptionText())
        {
            MentalityDescriptionText().Text(L"Keeps control under pressure.");
        }

        if (PhysicalDescriptionText())
        {
            PhysicalDescriptionText().Text(L"Breaks away from contests quickly.");
        }

        UpdateTraitSummary();
    }

    hstring XFactorPage::PageTitle()
    {
        return m_pageTitle;
    }

    void XFactorPage::PageTitle(hstring const& value)
    {
        m_pageTitle = value;
    }

    void XFactorPage::MentalityComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (!m_isPageReady)
        {
            return;
        }

        if (!MentalityComboBox() || !MentalityDescriptionText())
        {
            return;
        }

        auto selectedItem = MentalityComboBox().SelectedItem();
        if (!selectedItem)
        {
            return;
        }

        auto item = selectedItem.try_as<ComboBoxItem>();
        if (!item)
        {
            return;
        }

        auto value = unbox_value<hstring>(item.Content());

        if (value == L"Extroverted")
            MentalityDescriptionText().Text(L"Social, popular, high morale");
        else if (value == L"Outspoken")
            MentalityDescriptionText().Text(L"Bold, controversial and high risk personality");
        else if (value == L"Introverted")
            MentalityDescriptionText().Text(L"Quiet, focused, intelligent");
        else if (value == L"Professional")
            MentalityDescriptionText().Text(L"Extremely disciplined");

        UpdateTraitSummary();
    }

    void XFactorPage::PhysicalComboBox_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (!m_isPageReady)
        {
            return;
        }

        if (!PhysicalComboBox() || !PhysicalDescriptionText())
        {
            return;
        }

        auto selectedItem = PhysicalComboBox().SelectedItem();
        if (!selectedItem)
        {
            return;
        }

        auto item = selectedItem.try_as<ComboBoxItem>();
        if (!item)
        {
            return;
        }

        auto value = unbox_value<hstring>(item.Content());

        if (value == L"Explosive Speed")
            PhysicalDescriptionText().Text(L"Quick acceleration.");
        else if (value == L"Elite Endurance")
            PhysicalDescriptionText().Text(L"Lasts longer in matches.");
        else if (value == L"Strong Marking")
            PhysicalDescriptionText().Text(L"Dominates aerial contests.");
        else if (value == L"Powerful Frame")
            PhysicalDescriptionText().Text(L"Strong in contact.");
        else if (value == L"Booming Kick")
            PhysicalDescriptionText().Text(L"Huge distance kicking");

        UpdateTraitSummary();
    }

    void XFactorPage::WeaknessCheckBox_Changed(IInspectable const&, RoutedEventArgs const&)
    {
        if (!m_isPageReady)
        {
            return;
        }

        UpdateTraitSummary();
    }

    hstring XFactorPage::GetSelectedComboValue(ComboBox const& comboBox)
    {
        if (!comboBox || !comboBox.SelectedItem())
        {
            return L"";
        }

        if (auto item = comboBox.SelectedItem().try_as<ComboBoxItem>())
        {
            return unbox_value<hstring>(item.Content());
        }

        return L"";
    }

    hstring XFactorPage::BuildWeaknessSummary()
    {
        std::wstring weaknesses;

        if (WeaknessKick().IsChecked() && WeaknessKick().IsChecked().Value())
            weaknesses += L"Inconsistent Kicking, ";
        if (WeaknessFitness().IsChecked() && WeaknessFitness().IsChecked().Value())
            weaknesses += L"Low Endurance, ";
        if (WeaknessDecision().IsChecked() && WeaknessDecision().IsChecked().Value())
            weaknesses += L"Poor Decision Making, ";
        if (WeaknessPressure().IsChecked() && WeaknessPressure().IsChecked().Value())
            weaknesses += L"Struggles Under Pressure, ";

        if (weaknesses.empty())
        {
            return L"None selected";
        }

        weaknesses.erase(weaknesses.size() - 2);
        return hstring(weaknesses);
    }

    void XFactorPage::UpdateTraitSummary()
    {
        if (!TraitSummaryText())
        {
            return;
        }

        std::wstring mentality = L"Composed";
        std::wstring physical = L"Explosive Speed";
        if (MentalityComboBox() && MentalityComboBox().SelectedItem())
        {
            if (auto item = MentalityComboBox().SelectedItem().try_as<ComboBoxItem>())
            {
                mentality = unbox_value<hstring>(item.Content()).c_str();
            }
        }

        if (PhysicalComboBox() && PhysicalComboBox().SelectedItem())
        {
            if (auto item = PhysicalComboBox().SelectedItem().try_as<ComboBoxItem>())
            {
                physical = unbox_value<hstring>(item.Content()).c_str();
            }
        }

        std::wstring summary = L"Mentality: " + mentality + L"\nPhysical: " + physical;
        auto weaknesses = BuildWeaknessSummary();

        if (weaknesses != L"None selected")
        {
            summary += L"\nWeaknesses: " + std::wstring(weaknesses);
        }
        else
        {
            summary += L"\nWeaknesses: None selected";
        }

        TraitSummaryText().Text(hstring(summary));
    }

    void XFactorPage::BackButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (Frame().CanGoBack())
        {
            Frame().GoBack();
        }
    }

    void XFactorPage::ContinueButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        thefootballife::g_playerProfileState.mentalityXFactor = GetSelectedComboValue(MentalityComboBox());
        thefootballife::g_playerProfileState.physicalXFactor = GetSelectedComboValue(PhysicalComboBox());
        thefootballife::g_playerProfileState.weaknesses = BuildWeaknessSummary();

        Frame().Navigate(
            winrt::Windows::UI::Xaml::Interop::TypeName{
                L"thefootballife.ConfirmPlayerPage",
                winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
            }
        );
    }
}
