package com.saisai.catonpublisher.adapter;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;

import com.saisai.catonpublisher.fragment.LiveFragment;
import com.saisai.catonpublisher.fragment.SettingFragment;

public class MainVpAdapter extends FragmentPagerAdapter {

    public MainVpAdapter(FragmentManager fm) {
        super(fm);
    }

    @Override
    public Fragment getItem(int position) {
        if (position == 0) {
            return new LiveFragment();
        } else {
            return new SettingFragment();
        }
    }

    @Override
    public int getCount() {
        return 2;
    }
}
