package com.saisai.catonpublisher.fragment;

import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.saisai.catonpublisher.Constants;
import com.saisai.catonpublisher.R;
import com.saisai.catonpublisher.activity.Camera2Activity;
import com.saisai.catonpublisher.activity.LiveConfigActivity;
import com.saisai.catonpublisher.newencode.NewPublisherConfig;
import com.saisai.catonpublisher.util.PixelUtil;
import com.saisai.catonpublisher.util.SPUtils;

import java.util.List;

import de.hdodenhof.circleimageview.CircleImageView;

public class LiveFragment extends Fragment {

    private RecyclerView mRvLive;
    private List<NewPublisherConfig> mData;
    private LiveAdapter mAdapter;

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_live, container, false);
        mRvLive = view.findViewById(R.id.rv_live);
        mData = SPUtils.getLiveData();
        initRecycle();
        return view;
    }

    private void initRecycle() {
        GridLayoutManager layoutManager = new GridLayoutManager(getContext(), 2, GridLayoutManager.VERTICAL, false);
        mRvLive.setLayoutManager(layoutManager);
        final int pxOut = PixelUtil.dp2px(20f);
        mRvLive.addItemDecoration(new RecyclerView.ItemDecoration() {
            @Override
            public void getItemOffsets(Rect outRect, View view, RecyclerView parent, RecyclerView.State state) {
                super.getItemOffsets(outRect, view, parent, state);
                outRect.left = pxOut;
                outRect.right = pxOut;
                outRect.top = pxOut;
                outRect.bottom = pxOut;
            }
        });
        mAdapter = new LiveAdapter();
        mRvLive.setAdapter(mAdapter);
    }

    @Override
    public void onResume() {
        super.onResume();
        mData = SPUtils.getLiveData();
        mAdapter.notifyDataSetChanged();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 1) {
            mData = SPUtils.getLiveData();
            mAdapter.notifyDataSetChanged();
        }
    }

    class LiveAdapter extends RecyclerView.Adapter<LiveAdapter.LiveItemHolder> {

        @Override
        public LiveItemHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_live, parent, false);
            return new LiveItemHolder(view);
        }

        @Override
        public void onBindViewHolder(LiveItemHolder holder, final int position) {
            if (mData != null && mData.get(position) != null) {
                final NewPublisherConfig newPublisherConfig = mData.get(position);
                holder.mLiveTitle.setText(newPublisherConfig.liveTitle);
                if (position != 0) {
                    if (TextUtils.isEmpty(newPublisherConfig.imageSrc)) {
                        holder.mLiveCover.setImageResource(R.drawable.icon_default_image);
                    } else {
                        Bitmap bitmap = SPUtils.getBitmap(getContext(), newPublisherConfig.imageSrc);
                        if (bitmap != null) {
                            holder.mLiveCover.setImageBitmap(bitmap);
                        } else {
                            holder.mLiveCover.setImageResource(R.drawable.icon_default_image);
                        }
                    }
                } else {
                    holder.mLiveTitle.setText(getActivity().getString(R.string.add_live));
                    holder.mLiveCover.setImageResource(R.drawable.icon_add_circle);
                }
                holder.itemView.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (position == 0) {
                            //跳转到配置页面
                            Intent intent = new Intent(getContext(), LiveConfigActivity.class);
                            intent.putExtra(Constants.KEY_TITLE, getActivity().getString(R.string.add_live));
                            startActivityForResult(intent, 1);
                        } else {
                            //跳转到摄像页面
                            Intent intent = new Intent(getContext(), Camera2Activity.class);
                            intent.putExtra(Constants.KEY_TITLE, newPublisherConfig.liveTitle);
                            intent.putExtra(Constants.KEY_LIVE_ID, newPublisherConfig.id);
                            startActivity(intent);
                        }
                    }
                });
            }
        }

        @Override
        public int getItemCount() {
            return mData.size();
        }

        class LiveItemHolder extends RecyclerView.ViewHolder {
            private CircleImageView mLiveCover;
            private TextView mLiveTitle;
            public LiveItemHolder(View itemView) {
                super(itemView);
                mLiveCover = itemView.findViewById(R.id.live_cover);
                mLiveTitle = itemView.findViewById(R.id.live_title);
            }
        }
    }
}
