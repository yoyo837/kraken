/*
 * Copyright (C) 2020 Alibaba Inc. All rights reserved.
 * Author: Kraken Team.
 */

#include "iframe_element.h"

namespace kraken::binding::jsc {

JSIframeElement *JSIframeElement::instance(JSContext *context) {
  static std::unordered_map<JSContext *, JSIframeElement *> instanceMap{};
  if (!instanceMap.contains(context)) {
    instanceMap[context] = new JSIframeElement(context);
  }
  return instanceMap[context];
}

JSIframeElement::JSIframeElement(JSContext *context) : JSElement(context) {}
JSObjectRef JSIframeElement::instanceConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argumentCount,
                                                 const JSValueRef *arguments, JSValueRef *exception) {
  auto instance = new IframeElementInstance(this);
  return instance->object;
}

JSIframeElement::IframeElementInstance::IframeElementInstance(JSIframeElement *jsAnchorElement)
  : ElementInstance(jsAnchorElement, "iframe"), nativeIframeElement(new NativeIframeElement(nativeElement)) {
  JSStringRef canvasTagNameStringRef = JSStringCreateWithUTF8CString("iframe");
  NativeString tagName{};
  tagName.string = JSStringGetCharactersPtr(canvasTagNameStringRef);
  tagName.length = JSStringGetLength(canvasTagNameStringRef);

  const int32_t argsLength = 1;
  auto **args = new NativeString *[argsLength];
  args[0] = tagName.clone();

  foundation::UICommandTaskMessageQueue::instance(_hostClass->context->getContextId())
    ->registerCommand(eventTargetId, UICommandType::createElement, args, argsLength, nativeIframeElement);
}

std::vector<JSStringRef> &JSIframeElement::IframeElementInstance::getIframeElementPropertyNames() {
  static std::vector<JSStringRef> propertyNames{
    JSStringCreateWithUTF8CString("src"), JSStringCreateWithUTF8CString("type"), JSStringCreateWithUTF8CString("play")};
  return propertyNames;
}

const std::unordered_map<std::string, JSIframeElement::IframeElementInstance::IframeProperty> &
JSIframeElement::IframeElementInstance::getIframeElementPropertyMap() {
  static std::unordered_map<std::string, IframeProperty> propertyMap{
    {"width", IframeProperty::kWidth},
    {"height", IframeProperty::kHeight},
    {"contentWindow", IframeProperty::kContentWindow},
    {"postMessage", IframeProperty::kPostMessage},
  };
  return propertyMap;
}

JSValueRef JSIframeElement::IframeElementInstance::getProperty(std::string &name, JSValueRef *exception) {
  auto propertyMap = getIframeElementPropertyMap();
  if (propertyMap.contains(name)) {
    auto property = propertyMap[name];
    switch (property) {
    case IframeProperty::kWidth:
      return JSValueMakeNumber(_hostClass->ctx, _width);
    case IframeProperty::kHeight:
      return JSValueMakeNumber(_hostClass->ctx, _height);
    case IframeProperty::kContentWindow:
      // TODO: support contentWindow property.
      break;
    case IframeProperty::kPostMessage:
      if (_postMessage == nullptr) {
        _postMessage = propertyBindingFunction(_hostClass->context, this, "postMessage", postMessage);
        JSValueProtect(_hostClass->ctx, _postMessage);
      }
      return _postMessage;
    }
  }

  return ElementInstance::getProperty(name, exception);
}

void JSIframeElement::IframeElementInstance::setProperty(std::string &name, JSValueRef value, JSValueRef *exception) {
  auto propertyMap = getIframeElementPropertyMap();

  if (propertyMap.contains(name)) {
    auto property = propertyMap[name];
    switch (property) {
    case IframeProperty::kWidth: {
      _width = JSValueToNumber(_hostClass->ctx, value, exception);

      NativeString **args = new NativeString *[2];

      NativeString propertyKey{};
      STD_STRING_TO_NATIVE_STRING("width", propertyKey);

      NativeString propertyValue{};
      STD_STRING_TO_NATIVE_STRING(std::to_string(_width).c_str(), propertyValue);

      foundation::UICommandTaskMessageQueue::instance(_hostClass->contextId)
        ->registerCommand(eventTargetId, UICommandType::setProperty, args, 2, nullptr);
      break;
    }
    case IframeProperty::kHeight: {
      _height = JSValueToNumber(_hostClass->ctx, value, exception);

      NativeString **args = new NativeString *[2];

      NativeString propertyKey{};
      STD_STRING_TO_NATIVE_STRING("height", propertyKey);

      NativeString propertyValue{};
      STD_STRING_TO_NATIVE_STRING(std::to_string(_height).c_str(), propertyValue);

      foundation::UICommandTaskMessageQueue::instance(_hostClass->contextId)
        ->registerCommand(eventTargetId, UICommandType::setProperty, args, 2, nullptr);
      break;
    }
    default:
      break;
    }
  } else {
    NodeInstance::setProperty(name, value, exception);
  }
}

void JSIframeElement::IframeElementInstance::getPropertyNames(JSPropertyNameAccumulatorRef accumulator) {
  ElementInstance::getPropertyNames(accumulator);

  for (auto &property : getIframeElementPropertyNames()) {
    JSPropertyNameAccumulatorAddName(accumulator, property);
  }
}

JSIframeElement::IframeElementInstance::~IframeElementInstance() {
  delete nativeIframeElement;
  if (_postMessage != nullptr) JSValueUnprotect(_hostClass->ctx, _postMessage);
}

JSValueRef JSIframeElement::IframeElementInstance::postMessage(JSContextRef ctx, JSObjectRef function,
                                                               JSObjectRef thisObject, size_t argumentCount,
                                                               const JSValueRef *arguments, JSValueRef *exception) {
  if (argumentCount < 1) {
    JSC_THROW_ERROR(ctx, "Failed to execute 'postMessage' on 'IframeElement: 1 arguments required.'", exception);
    return nullptr;
  }

  if (!JSValueIsString(ctx, arguments[0])) {
    JSC_THROW_ERROR(ctx, "Failed to execute 'postMessage' on 'IframeElement: first arguments should be string'", exception);
    return nullptr;
  }

  JSStringRef messageStringRef = JSValueToStringCopy(ctx, arguments[0], exception);
  NativeString message{};
  message.string = JSStringGetCharactersPtr(messageStringRef);
  message.length = JSStringGetLength(messageStringRef);

  auto instance = reinterpret_cast<JSIframeElement::IframeElementInstance *>(JSObjectGetPrivate(function));
  instance->nativeIframeElement->postMessage(instance->nativeIframeElement, &message);

  return nullptr;
}

} // namespace kraken::binding::jsc
