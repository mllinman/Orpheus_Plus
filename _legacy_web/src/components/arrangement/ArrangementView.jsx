import React, { useRef, useCallback, useEffect, useState } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';
import TrackHeader from './TrackHeader';
import TrackLane from './TrackLane';
import TimelineRuler from './TimelineRuler';
import Playhead from './Playhead';
import AutomationLane from './AutomationLane';

export default function ArrangementView() {
  const { tracks, setPlayheadPosition, bpm, splitClip } = useProjectStore();
  const { horizontalZoom, verticalZoom, trackHeaderWidth } = useUIStore();
  const scrollRef = useRef(null);
  const headerScrollRef = useRef(null);

  const trackHeight = 80 * verticalZoom;
  const totalBars = 64;
  const totalBeats = totalBars * 4;
  const totalWidth = totalBeats * horizontalZoom;

  // Sync vertical scroll between headers and lanes
  const handleScroll = useCallback(() => {
    if (headerScrollRef.current && scrollRef.current) {
      headerScrollRef.current.scrollTop = scrollRef.current.scrollTop;
    }
  }, []);

  // Handle zoom and scrolling with mouse wheel
  const handleWheel = useCallback((e) => {
    // Ctrl/Cmd + Wheel = Zoom
    if (e.ctrlKey || e.metaKey) {
      e.preventDefault();
      // If Shift also pressed, maybe vertical zoom? For now just horizontal
      const { zoomIn, zoomOut } = useUIStore.getState();
      if (e.deltaY < 0) zoomIn();
      else zoomOut();
      return;
    }

    // Shift + Wheel = Horizontal Scroll
    if (e.shiftKey) {
      e.preventDefault();
      if (scrollRef.current) {
        scrollRef.current.scrollLeft += e.deltaY;
      }
      return;
    }

    // Default Vertical Scroll (native behavior)
  }, []);

  // Pan Tool (Middle Click or Space+Drag)
  const [isPanning, setIsPanning] = useState(false);
  const lastMousePos = useRef({ x: 0, y: 0 });

  const handleMouseDown = (e) => {
    // Middle mouse button (1)
    if (e.button === 1) {
      e.preventDefault();
      setIsPanning(true);
      lastMousePos.current = { x: e.clientX, y: e.clientY };
    }
  };

  const handleMouseMove = (e) => {
    if (isPanning && scrollRef.current) {
      const dx = e.clientX - lastMousePos.current.x;
      const dy = e.clientY - lastMousePos.current.y;
      
      scrollRef.current.scrollLeft -= dx;
      scrollRef.current.scrollTop -= dy;
      
      lastMousePos.current = { x: e.clientX, y: e.clientY };
    }
  };

  const handleMouseUp = () => {
    setIsPanning(false);
  };

  // Handle drop of stems
  const handleDrop = useCallback((e) => {
    e.preventDefault();
    const stemData = e.dataTransfer.getData('application/orpheus-stem');
    if (stemData) {
      try {
        const { label, color } = JSON.parse(stemData);
        const store = useProjectStore.getState();
        const trackId = store.addTrack('audio');
        store.renameTrack(trackId, `${label} (Stem)`);
        store.setTrackColor(trackId, color);
        
        // Add a demo clip
        store.addClip(trackId, {
          id: Date.now().toString(),
          type: 'audio',
          name: `${label} Clip`,
          startBeat: 0,
          lengthBeats: 16,
          offset: 0,
          gain: 1,
          bufferId: null
        });
      } catch (err) {
        console.error('Invalid stem data', err);
      }
    }
  }, []);

  // Handle click on background to move playhead
  const handleBackgroundClick = (e) => {
    // Ignore if clicking a child element (like a clip) or if panning
    if (isPanning || e.target.closest('.audio-clip') || e.target.closest('.automation-point')) return;
    
    // Only set playhead if clicking on the timeline area (not headers)
    if (e.target.closest('.arrangement-headers')) return;

    const rect = e.currentTarget.getBoundingClientRect();
    const scrollLeft = e.currentTarget.scrollLeft;
    const x = e.clientX - rect.left + scrollLeft;
    
    // Calculate time
    // x is pixels. pixelsPerBeat = horizontalZoom
    const beat = x / horizontalZoom;
    const time = (beat / bpm) * 60;
    
    setPlayheadPosition(time);
  };

  return (
    <div 
      className="arrangement-view" 
      onWheel={handleWheel}
      onMouseDown={handleMouseDown}
      onMouseMove={handleMouseMove}
      onMouseUp={handleMouseUp}
      onMouseLeave={handleMouseUp}
      onDragOver={(e) => e.preventDefault()}
      onDrop={handleDrop}
      style={{ cursor: isPanning ? 'grabbing' : 'auto' }}
    >
      {/* Track Headers Column */}
      <div className="arrangement-headers" style={{ width: trackHeaderWidth }}>
        <div className="arrangement-header-spacer">
          <span className="text-muted" style={{ fontSize: 'var(--text-xs)' }}>TRACKS</span>
        </div>
        <div className="arrangement-headers-scroll" ref={headerScrollRef}>
          {tracks.map((track, index) => (
            <TrackHeader key={track.id} track={track} index={index} height={trackHeight} />
          ))}
          <AddTrackButton />
        </div>
      </div>

      {/* Resize handle */}
      <ResizeHandle />

      {/* Timeline + Lanes */}
      <div className="arrangement-content">
        <TimelineRuler
          totalBeats={totalBeats}
          pixelsPerBeat={horizontalZoom}
          scrollLeft={scrollRef.current?.scrollLeft || 0}
        />
        <div
          className="arrangement-lanes-scroll"
          ref={scrollRef}
          onScroll={handleScroll}
          onClick={handleBackgroundClick}
        >
          <div className="arrangement-lanes" style={{ width: totalWidth, minWidth: '100%' }}>
            {tracks.map((track, i) => (
              <React.Fragment key={track.id}>
                <TrackLane
                  track={track}
                  trackIndex={i}
                  height={trackHeight}
                  pixelsPerBeat={horizontalZoom}
                  totalWidth={totalWidth}
                />
                {(track.automationLanes || []).map((lane, li) => (
                  <AutomationLane key={`${track.id}-auto-${li}`} trackId={track.id} laneIndex={li} />
                ))}
              </React.Fragment>
            ))}
          </div>
          <Playhead pixelsPerBeat={horizontalZoom} totalHeight={tracks.length * trackHeight} />
        </div>
      </div>
    </div>
  );
}

function AddTrackButton() {
  const addTrack = useProjectStore(s => s.addTrack);
  const [open, setOpen] = useState(false);

  return (
    <div className="add-track-container">
      <button className="btn btn-ghost add-track-btn" onClick={() => setOpen(!open)}>
        + Add Track
      </button>
      {open && (
        <div className="dropdown-menu" style={{ position: 'relative', top: 0 }}>
          <div className="dropdown-item" onClick={() => { addTrack('audio'); setOpen(false); }}>
            <span>🎵 Audio Track</span>
          </div>
          <div className="dropdown-item" onClick={() => { addTrack('midi'); setOpen(false); }}>
            <span>🎹 MIDI Track</span>
          </div>
          <div className="dropdown-item" onClick={() => { addTrack('midi'); setOpen(false); }}>
            <span>🎸 Instrument Track</span>
          </div>
        </div>
      )}
    </div>
  );
}

function ResizeHandle() {
  const { trackHeaderWidth, setTrackHeaderWidth } = useUIStore();
  const [dragging, setDragging] = useState(false);
  const startX = useRef(0);
  const startWidth = useRef(0);

  const onMouseDown = (e) => {
    e.preventDefault();
    setDragging(true);
    startX.current = e.clientX;
    startWidth.current = trackHeaderWidth;
  };

  useEffect(() => {
    if (!dragging) return;
    const onMove = (e) => {
      const delta = e.clientX - startX.current;
      setTrackHeaderWidth(startWidth.current + delta);
    };
    const onUp = () => setDragging(false);
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, [dragging, setTrackHeaderWidth]);

  return <div className={`resize-handle ${dragging ? 'dragging' : ''}`} onMouseDown={onMouseDown} />;
}
