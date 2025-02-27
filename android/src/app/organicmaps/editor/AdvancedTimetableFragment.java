package app.organicmaps.editor;

import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.EditText;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.util.Constants;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.UiUtils;

public class AdvancedTimetableFragment extends BaseMwmFragment
                                       implements View.OnClickListener, TimetableProvider
{
  private boolean mIsExampleShown;
  private EditText mInput;
  private WebView mExample;
  private TextView mExamplesTitle;
  @Nullable
  private String mInitTimetables;
  @Nullable
  TimetableChangedListener mListener;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_timetable_advanced, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    initViews(view);
    refreshTimetables();
    showExample(false);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    refreshTimetables();
  }

  private void initViews(View view)
  {
    view.findViewById(R.id.examples).setOnClickListener(this);
    mInput = view.findViewById(R.id.et__timetable);
    mExample = view.findViewById(R.id.wv__examples);
    mExample.loadUrl(Constants.Url.OPENING_HOURS_MANUAL);
    mExamplesTitle = view.findViewById(R.id.tv__examples_title);
    setExampleDrawables(R.drawable.ic_type_text, R.drawable.ic_expand_more);
    setTextChangedListener(mInput, mListener);
  }

  private void showExample(boolean show)
  {
    mIsExampleShown = show;
    if (mIsExampleShown)
    {
      UiUtils.show(mExample);
      setExampleDrawables(R.drawable.ic_type_text, R.drawable.ic_expand_less);
    }
    else
    {
      UiUtils.hide(mExample);
      setExampleDrawables(R.drawable.ic_type_text, R.drawable.ic_expand_more);
    }
  }

  private void setExampleDrawables(@DrawableRes int left, @DrawableRes int right)
  {
    mExamplesTitle.setCompoundDrawablesRelativeWithIntrinsicBounds(Graphics.tint(requireActivity(), left, R.attr.colorAccent), null,
                                                           Graphics.tint(requireActivity(), right, R.attr.colorAccent), null);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.examples:
      showExample(!mIsExampleShown);
    }
  }

  @Nullable
  @Override
  public String getTimetables()
  {
    return mInput.getText().toString();
  }

  @Override
  public void setTimetables(@Nullable String timetables)
  {
    mInitTimetables = timetables;
    refreshTimetables();
  }

  private void refreshTimetables()
  {
    if (mInput == null || mInitTimetables == null)
      return;

    mInput.setText(mInitTimetables);
    mInput.requestFocus();
    InputUtils.showKeyboard(mInput);
  }

  void setTimetableChangedListener(@NonNull TimetableChangedListener listener)
  {
    mListener = listener;
    setTextChangedListener(mInput, mListener);
  }

  private static void setTextChangedListener(@Nullable EditText input,
                                             @Nullable TimetableChangedListener listener)
  {
    if (input == null || listener == null)
      return;

    input.addTextChangedListener(new TextWatcher()
    {
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {}

      @Override
      public void afterTextChanged(Editable s)
      {
        listener.onTimetableChanged(s.toString());
      }
    });
  }
}
