#!/usr/bin/env python3
# Simplified detector.py

import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
from sensor_msgs.msg import Image, CameraInfo
from geometry_msgs.msg import PointStamped
import message_filters
from cv_bridge import CvBridge
import cv2
import numpy as np
import tf2_ros
from tf2_geometry_msgs import do_transform_point
from isaac_moveit_package.srv import CubePosition

class RedCubeTracker(Node):
    def __init__(self):
        super().__init__("red_cube_tracker")

        self.robot_frame = "panda_link0"
        self.camera_frame = "Camera"
        self.rgb_topic = "/rgb"
        self.depth_topic = "/depth"
        self.camera_info_topic = "/camera_info"

        self.bridge = CvBridge()
        self.fx = self.fy = self.cx = self.cy = None
        self.last_pos = {"x":0.0,"y":0.0,"z":0.0}

        self.tf_buffer = tf2_ros.Buffer()
        self.tf_listener = tf2_ros.TransformListener(self.tf_buffer,self)

        self.create_subscription(CameraInfo,self.camera_info_topic,self.info_cb,10)

        rgb_sub = message_filters.Subscriber(self,Image,self.rgb_topic)
        depth_sub = message_filters.Subscriber(self,Image,self.depth_topic)
        sync = message_filters.ApproximateTimeSynchronizer([rgb_sub,depth_sub],10,0.05)
        sync.registerCallback(self.image_cb)
        self.sync=sync

        self.point_pub=self.create_publisher(PointStamped,"/red_cube/position",10)
        self.image_pub=self.create_publisher(Image,"/red_cube/image",10)
        self.create_service(CubePosition,"/cube_position",self.handle_cube_position)

    def handle_cube_position(self,request,response):
        response.x=self.last_pos["x"]
        response.y=self.last_pos["y"]
        response.z=self.last_pos["z"]
        return response

    def info_cb(self,msg):
        self.fx,self.fy,self.cx,self.cy=msg.k[0],msg.k[4],msg.k[2],msg.k[5]

    def image_cb(self,rgb_msg,depth_msg):
        if self.fx is None: return
        rgb=self.bridge.imgmsg_to_cv2(rgb_msg,"bgr8")
        depth=np.asarray(self.bridge.imgmsg_to_cv2(depth_msg,"passthrough"),dtype=np.float32)
        hsv=cv2.cvtColor(rgb,cv2.COLOR_BGR2HSV)
        mask=cv2.inRange(hsv,(0,120,70),(10,255,255))|cv2.inRange(hsv,(170,120,70),(180,255,255))
        mask=cv2.erode(mask,None,iterations=2); mask=cv2.dilate(mask,None,iterations=2)
        contours,_=cv2.findContours(mask,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
        if contours:
            c=max(contours,key=cv2.contourArea)
            if cv2.contourArea(c)>200:
                M=cv2.moments(c)
                if M["m00"]>0:
                    u,v=int(M["m10"]/M["m00"]),int(M["m01"]/M["m00"])
                    Z=self._sample_depth(depth,rgb.shape,u,v)
                    if Z is not None:
                        pt=PointStamped()
                        pt.header=rgb_msg.header
                        pt.header.frame_id=self.camera_frame
                        pt.point.x=(u-self.cx)*Z/self.fx
                        pt.point.y=(v-self.cy)*Z/self.fy
                        pt.point.z=Z
                        pr=self._to_robot_frame(pt)
                        cv2.circle(rgb,(u,v),5,(0,255,0),-1)
                        if pr:
                            self.point_pub.publish(pr)
                            self.last_pos["x"]=pr.point.x
                            self.last_pos["y"]=pr.point.y
                            self.last_pos["z"]=pr.point.z
                            txt=f'X:{pr.point.x:.3f} Y:{pr.point.y:.3f} Z:{pr.point.z:.3f}'
                            cv2.putText(rgb,txt,(10,30),cv2.FONT_HERSHEY_SIMPLEX,0.6,(0,255,0),2)
        out=self.bridge.cv2_to_imgmsg(rgb,"bgr8"); out.header=rgb_msg.header
        self.image_pub.publish(out)

    def _to_robot_frame(self,pt):
        for stamp in (pt.header.stamp,Time().to_msg()):
            try:
                tf=self.tf_buffer.lookup_transform(self.robot_frame,pt.header.frame_id,stamp,timeout=Duration(seconds=0.1))
                return do_transform_point(pt,tf)
            except Exception:
                pass
        return None

    def _sample_depth(self,depth,rgb_shape,u,v):
        dh,dw=depth.shape[:2]; rh,rw=rgb_shape[:2]
        du,dv=int(u*dw/rw),int(v*dh/rh)
        patch=depth[max(0,dv-3):dv+4,max(0,du-3):du+4]
        valid=patch[np.isfinite(patch)&(patch>0)]
        return float(np.median(valid)) if valid.size else None

def main():
    rclpy.init()
    node=RedCubeTracker()
    try:rclpy.spin(node)
    except KeyboardInterrupt:pass
    node.destroy_node()
    rclpy.shutdown()

if __name__=="__main__":
    main()