// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_RENDERER_AUTOFILL_AGENT_H_
#define COMPONENTS_AUTOFILL_RENDERER_AUTOFILL_AGENT_H_

#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/time.h"
#include "base/timer.h"
#include "components/autofill/common/forms_seen_state.h"
#include "components/autofill/renderer/form_cache.h"
#include "components/autofill/renderer/page_click_listener.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebAutofillClient.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFormElement.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputElement.h"

namespace WebKit {
class WebNode;
class WebView;
}

namespace autofill {

struct FormData;
struct FormFieldData;
struct WebElementDescriptor;
class PasswordAutofillAgent;

// AutofillAgent deals with Autofill related communications between WebKit and
// the browser.  There is one AutofillAgent per RenderView.
// This code was originally part of RenderView.
// Note that Autofill encompasses:
// - single text field suggestions, that we usually refer to as Autocomplete,
// - password form fill, refered to as Password Autofill, and
// - entire form fill based on one field entry, referred to as Form Autofill.

class AutofillAgent : public content::RenderViewObserver,
                      public PageClickListener,
                      public WebKit::WebAutofillClient {
 public:
  // PasswordAutofillAgent is guaranteed to outlive AutofillAgent.
  AutofillAgent(content::RenderView* render_view,
                PasswordAutofillAgent* password_autofill_manager);
  virtual ~AutofillAgent();

 private:
  enum AutofillAction {
    AUTOFILL_NONE,     // No state set.
    AUTOFILL_FILL,     // Fill the Autofill form data.
    AUTOFILL_PREVIEW,  // Preview the Autofill form data.
  };

  // RenderView::Observer:
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
  virtual void DidFinishDocumentLoad(WebKit::WebFrame* frame) OVERRIDE;
  virtual void DidStartProvisionalLoad(WebKit::WebFrame* frame) OVERRIDE;
  virtual void DidFailProvisionalLoad(
      WebKit::WebFrame* frame,
      const WebKit::WebURLError& error) OVERRIDE;
  virtual void DidCommitProvisionalLoad(WebKit::WebFrame* frame,
                                        bool is_new_navigation) OVERRIDE;
  virtual void FrameDetached(WebKit::WebFrame* frame) OVERRIDE;
  virtual void WillSubmitForm(WebKit::WebFrame* frame,
                              const WebKit::WebFormElement& form) OVERRIDE;
  virtual void ZoomLevelChanged() OVERRIDE;
  virtual void DidChangeScrollOffset(WebKit::WebFrame* frame) OVERRIDE;
  virtual void FocusedNodeChanged(const WebKit::WebNode& node) OVERRIDE;

  // PageClickListener:
  virtual void InputElementClicked(const WebKit::WebInputElement& element,
                                   bool was_focused,
                                   bool is_focused) OVERRIDE;
  virtual void InputElementLostFocus() OVERRIDE;

  // WebKit::WebAutofillClient:
  virtual void didAcceptAutofillSuggestion(const WebKit::WebNode& node,
                                           const WebKit::WebString& value,
                                           const WebKit::WebString& label,
                                           int item_id,
                                           unsigned index) OVERRIDE;
  virtual void didSelectAutofillSuggestion(const WebKit::WebNode& node,
                                           const WebKit::WebString& value,
                                           const WebKit::WebString& label,
                                           int item_id) OVERRIDE;
  virtual void didClearAutofillSelection(const WebKit::WebNode& node) OVERRIDE;
  virtual void removeAutocompleteSuggestion(
      const WebKit::WebString& name,
      const WebKit::WebString& value) OVERRIDE;
  virtual void textFieldDidEndEditing(
      const WebKit::WebInputElement& element) OVERRIDE;
  virtual void textFieldDidChange(
      const WebKit::WebInputElement& element) OVERRIDE;
  virtual void textFieldDidReceiveKeyDown(
      const WebKit::WebInputElement& element,
      const WebKit::WebKeyboardEvent& event) OVERRIDE;
  virtual void didRequestAutocomplete(
      WebKit::WebFrame* frame,
      const WebKit::WebFormElement& form) OVERRIDE;
  virtual void setIgnoreTextChanges(bool ignore) OVERRIDE;
  virtual void didAssociateFormControls(
      const WebKit::WebVector<WebKit::WebNode>& nodes) OVERRIDE;

  void OnSuggestionsReturned(int query_id,
                             const std::vector<base::string16>& values,
                             const std::vector<base::string16>& labels,
                             const std::vector<base::string16>& icons,
                             const std::vector<int>& unique_ids);
  void OnFormDataFilled(int query_id, const FormData& form);
  void OnFieldTypePredictionsAvailable(
      const std::vector<FormDataPredictions>& forms);

  // For external Autofill selection.
  void OnSetAutofillActionFill();
  void OnClearForm();
  void OnSetAutofillActionPreview();
  void OnClearPreviewedForm();
  void OnSetNodeText(const base::string16& value);
  void OnAcceptDataListSuggestion(const base::string16& value);
  void OnAcceptPasswordAutofillSuggestion(const base::string16& value);
  void OnGetAllForms();

  // Called when interactive autocomplete finishes.
  void OnRequestAutocompleteResult(
      WebKit::WebFormElement::AutocompleteResult result,
      const FormData& form_data);

  // Called when an autocomplete request succeeds or fails with the |result|.
  void FinishAutocompleteRequest(
      WebKit::WebFormElement::AutocompleteResult result);

  // Called when the Autofill server hints that this page should be filled using
  // Autocheckout. All the relevant form fields in |form_data| will be filled
  // and then element specified by |element_descriptor| will be clicked to
  // proceed to the next step of the form.
  void OnFillFormsAndClick(const std::vector<FormData>& form_data,
                           const WebElementDescriptor& element_descriptor);

  // Called when |topmost_frame_| is supported for Autocheckout.
  void OnAutocheckoutSupported();

  // Called when clicking an Autocheckout proceed element fails to do anything.
  void ClickFailed();

  // Called in a posted task by textFieldDidChange() to work-around a WebKit bug
  // http://bugs.webkit.org/show_bug.cgi?id=16976
  void TextFieldDidChangeImpl(const WebKit::WebInputElement& element);

  // Shows the autofill suggestions for |element|.
  // This call is asynchronous and may or may not lead to the showing of a
  // suggestion popup (no popup is shown if there are no available suggestions).
  // |autofill_on_empty_values| specifies whether suggestions should be shown
  // when |element| contains no text.
  // |requires_caret_at_end| specifies whether suggestions should be shown when
  // the caret is not after the last character in |element|.
  // |display_warning_if_disabled| specifies whether a warning should be
  // displayed to the user if Autofill has suggestions available, but cannot
  // fill them because it is disabled (e.g. when trying to fill a credit card
  // form on a non-secure website).
  void ShowSuggestions(const WebKit::WebInputElement& element,
                       bool autofill_on_empty_values,
                       bool requires_caret_at_end,
                       bool display_warning_if_disabled);

  // Queries the browser for Autocomplete and Autofill suggestions for the given
  // |element|.
  void QueryAutofillSuggestions(const WebKit::WebInputElement& element,
                                bool display_warning_if_disabled);

  // Combines DataList suggestion entries with the autofill ones and show them
  // to the user.
  void CombineDataListEntriesAndShow(const WebKit::WebInputElement& element,
                                     const std::vector<base::string16>& values,
                                     const std::vector<base::string16>& labels,
                                     const std::vector<base::string16>& icons,
                                     const std::vector<int>& item_ids,
                                     bool has_autofill_item);

  // Sets the element value to reflect the selected |suggested_value|.
  void AcceptDataListSuggestion(const base::string16& suggested_value);

  // Queries the AutofillManager for form data for the form containing |node|.
  // |value| is the current text in the field, and |unique_id| is the selected
  // profile's unique ID.  |action| specifies whether to Fill or Preview the
  // values returned from the AutofillManager.
  void FillAutofillFormData(const WebKit::WebNode& node,
                            int unique_id,
                            AutofillAction action);

  // Fills |form| and |field| with the FormData and FormField corresponding to
  // |node|. Returns true if the data was found; and false otherwise.
  bool FindFormAndFieldForNode(
      const WebKit::WebNode& node,
      FormData* form,
      FormFieldData* field) WARN_UNUSED_RESULT;

  // Set |node| to display the given |value|.
  void SetNodeText(const base::string16& value, WebKit::WebInputElement* node);

  // Hides any currently showing Autofill UI in the renderer or browser.
  void HideAutofillUi();

  // Hides any currently showing Autofill UI in the browser only.
  void HideHostAutofillUi();

  void MaybeSendDynamicFormsSeen();

  // Send |AutofillHostMsg_MaybeShowAutocheckoutBubble| to browser if needed.
  void MaybeShowAutocheckoutBubble();

  FormCache form_cache_;

  PasswordAutofillAgent* password_autofill_agent_;  // WEAK reference.

  // The ID of the last request sent for form field Autofill.  Used to ignore
  // out of date responses.
  int autofill_query_id_;

  // The element corresponding to the last request sent for form field Autofill.
  WebKit::WebInputElement element_;

  // The form element currently requesting an interactive autocomplete.
  WebKit::WebFormElement in_flight_request_form_;

  // All the form elements seen in the top frame.
  std::vector<WebKit::WebFormElement> form_elements_;

  // The action to take when receiving Autofill data from the AutofillManager.
  AutofillAction autofill_action_;

  // Pointer to the current topmost frame.  Used in autocheckout flows so
  // elements can be clicked.
  WebKit::WebFrame* topmost_frame_;

  // Pointer to the WebView. Used to access page scale factor.
  WebKit::WebView* web_view_;

  // Should we display a warning if autofill is disabled?
  bool display_warning_if_disabled_;

  // Was the query node autofilled prior to previewing the form?
  bool was_query_node_autofilled_;

  // Have we already shown Autofill suggestions for the field the user is
  // currently editing?  Used to keep track of state for metrics logging.
  bool has_shown_autofill_popup_for_current_edit_;

  // If true we just set the node text so we shouldn't show the popup.
  bool did_set_node_text_;

  // Watchdog timer for clicking in Autocheckout flows.
  base::OneShotTimer<AutofillAgent> click_timer_;

  // Used to signal that we need to watch for loading failures in an
  // Autocheckout flow.
  bool autocheckout_click_in_progress_;

  // Whether or not |topmost_frame_| is whitelisted for Autocheckout.
  bool is_autocheckout_supported_;

  // Whether or not new forms/fields have been dynamically added
  // since the last loaded forms were sent to the browser process.
  bool has_new_forms_for_browser_;

  // Whether or not we should try to offer the user Autocheckout functionality
  // by sending |AutofillHostMsg_MaybeShowAutocheckoutBubble| to the browser.
  bool try_to_show_autocheckout_bubble_;

  // Whether or not to ignore text changes.  Useful for when we're committing
  // a composition when we are defocusing the WebView and we don't want to
  // trigger an autofill popup to show.
  bool ignore_text_changes_;

  // Timestamp of first time forms are seen.
  base::TimeTicks forms_seen_timestamp_;

  base::WeakPtrFactory<AutofillAgent> weak_ptr_factory_;

  friend class PasswordAutofillAgentTest;
  FRIEND_TEST_ALL_PREFIXES(ChromeRenderViewTest, FillFormElement);
  FRIEND_TEST_ALL_PREFIXES(ChromeRenderViewTest, SendForms);
  FRIEND_TEST_ALL_PREFIXES(ChromeRenderViewTest, SendDynamicForms);
  FRIEND_TEST_ALL_PREFIXES(ChromeRenderViewTest, ShowAutofillWarning);
  FRIEND_TEST_ALL_PREFIXES(PasswordAutofillAgentTest, WaitUsername);
  FRIEND_TEST_ALL_PREFIXES(PasswordAutofillAgentTest, SuggestionAccept);
  FRIEND_TEST_ALL_PREFIXES(PasswordAutofillAgentTest, SuggestionSelect);

  DISALLOW_COPY_AND_ASSIGN(AutofillAgent);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_RENDERER_AUTOFILL_AGENT_H_
