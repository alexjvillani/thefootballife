#include "pch.h"
#include "CreditPage.xaml.h"
#include "GameState.h"
#if __has_include("CreditPage.g.cpp")
#include "CreditPage.g.cpp"
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
	CreditPage::CreditPage()
	{
		InitializeComponent();
	}

	void CreditPage::BackButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		Frame().Navigate(
		winrt::Windows::UI::Xaml::Interop::TypeName{
			L"thefootballife.MainMenuPage",
			winrt::Windows::UI::Xaml::Interop::TypeKind::Custom
			}
		);
	}



}