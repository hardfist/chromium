// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SIMPLE_SIMPLE_WEB_CONTENTS_VIEW_DELEGATE_CREATOR_H_
#define CONTENT_SIMPLE_SIMPLE_WEB_CONTENTS_VIEW_DELEGATE_CREATOR_H_

namespace content {

class WebContents;
class WebContentsViewDelegate;

WebContentsViewDelegate* CreateSimpleWebContentsViewDelegate(
    WebContents* web_contents);

}  // namespace content

#endif //CONTENT_SIMPLE_SIMPLE_WEB_CONTENTS_VIEW_DELEGATE_CREATOR_H_
